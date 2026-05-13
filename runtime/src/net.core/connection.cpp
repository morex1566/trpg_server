#include "connection.h"

#include "net.core/gen/packet.generated.h"
#include "net.common/log.h"
#include "net.common/system_config.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <typeinfo>
#include <utility>

namespace net::core
{
    connection::connection(boost::asio::io_context& context, boost::asio::ip::tcp::socket&& client_socket, uint64_t connection_id,
        tbb::concurrent_queue<packet_recv_context>& queue) :
    strand(context),
    socket(std::move(client_socket)),
    connection_guid(connection_id),
    recv_queue(queue),
    curr_state(state::connected),
    is_writing(false)
    {
        SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::core::connection).name()));
    }

    connection::~connection()
    {
        boost::system::error_code error;
        socket.close(error);

        SPDLOG_INFO("destroy {} instance.", net::common::demangle(typeid(net::core::connection).name()));
    }

    void connection::send(packet_send_context&& context)
    {
        auto self = shared_from_this();

        boost::asio::post(strand, [self, context = std::move(context)]() mutable
        {
            // 송신 패킷 정상적?
            if (!self->try_validate_send_context(context)) return;

            // 송신 큐에 넣을 수 있음?
            if (!self->enqueue_send(std::move(context)))
            {
                self->close();
                return;
            }
        });
    }
        
    void connection::async_write()
    {
        auto self = shared_from_this();

        boost::asio::post(strand, [self]() mutable
        {
            // 클라이언트 끊김?
            if (self->curr_state == state::disconnected) return;

			// 이미 쓰는 중? async_write_batch이 끝나기 전에 또 async_write가 호출됐다는 뜻. 무시
            if (self->is_writing) return;

            // 쓸게 없음?
            if (self->send_queue.empty()) return;

            auto batch = self->create_send_context_batch();
            if (!batch) return;

            self->async_write_batch(batch);
        });
    }

    void connection::async_read()
    {
        auto self = shared_from_this();

        boost::asio::post(strand, [self]() mutable
        {
            // 클라이언트 끊김?
            if (self->curr_state == state::disconnected)
            {
                SPDLOG_WARN("connection disconnected.");
                self->close();
                return;
			}

            self->async_read_header();
        });
    }

    void connection::close()
    {
        auto self = shared_from_this();

        // close는 여러 경로에서 호출될 수 있으므로 strand에서 한 번만 socket을 닫음
        boost::asio::post(strand, [self]
        {
            if (self->curr_state == state::disconnected) return;

            boost::system::error_code error;
            self->socket.close(error);

            self->curr_state = state::disconnected;
        });
    }

    void connection::async_read_header()
    {
        auto self = shared_from_this();

        boost::asio::async_read
        (
            socket,
            boost::asio::buffer(&recv_prefix, sizeof(flatbuffers::uoffset_t)),
            boost::asio::bind_executor(strand, [self](boost::system::error_code error, std::size_t)
            {
                // Socket 에러?
                if (error)
                {
                    SPDLOG_WARN("socket error : {}.", error.message());
                    self->close();
                    return;
                }

                // 클라이언트 끊김?
                if (self->curr_state == state::disconnected)
                {
                    return;
                }

                // 패킷 사이즈 정상적? context 생성 가능?
                const auto prefix = self->recv_prefix;
                std::shared_ptr<packet_recv_context> context;
                if (!self->try_validate_recv_context(prefix, context))
                {
                    self->close();
                    return;
                }

				// 완료. Payload 읽기 시작
				self->async_read_payload(context);
            })
        );
    }

    void connection::async_read_payload(std::shared_ptr<packet_recv_context> context)
    {
        auto self = shared_from_this();

        boost::asio::async_read
        (
            socket,
            boost::asio::buffer(context->buffer.data() + sizeof(flatbuffers::uoffset_t), context->buffer.size() - sizeof(flatbuffers::uoffset_t)),
            boost::asio::bind_executor(strand, [self, context](boost::system::error_code error, std::size_t)
            {
                // Socket 에러?
                if (error)
                {
                    SPDLOG_WARN("socket error : {}.", error.message());
                    self->close();
                    return;
                }

                // 클라이언트 끊김?
                if (self->curr_state == state::disconnected)
                {
                    return;
                }

                // 패킷 정상적? 등록된 패킷?
                if (!self->try_validate_recv_context(*context))
                {
                    self->close();
                    return;
                }

                // 큐 꽉참?(연결 상태 안좋다는 뜻)
                if (!self->enqueue_recv(std::move(*context)))
                {
                    self->close();
                    return;
                }

                // 다음 패킷 읽기 시작
                self->async_read_header();
            })
        );
    }

    void connection::async_write_batch(std::shared_ptr<packet_send_context_batch> batch)
    {
        auto self = shared_from_this();

        is_writing = true;

        boost::asio::async_write
        (
            socket,
            batch->buffers,
            boost::asio::bind_executor(strand, [self, batch](boost::system::error_code error, std::size_t)
            {
                self->is_writing = false;

                if (error)
                {
                    SPDLOG_WARN("socket error : {}.", error.message());
                    self->close();
                    return;
                }
            })
        );
    }

    std::shared_ptr<connection::packet_send_context_batch> connection::create_send_context_batch()
    {
        auto batch = std::make_shared<packet_send_context_batch>();

		// send_queue에 있는 패킷들을 batch로 묶음
        while (!send_queue.empty())
        {
            batch->contexts.emplace_back(std::move(send_queue.front()));
            send_queue.pop();
        }

		// async_write에 전달할 buffer 목록 생성
        batch->buffers.reserve(batch->contexts.size());
        for (const auto& context : batch->contexts)
        {
            batch->buffers.emplace_back
            (
                boost::asio::buffer(context.buffer.data(), context.buffer.size())
            );
        }

        return batch;
    }

    bool connection::try_validate_recv_context(flatbuffers::uoffset_t prefix, std::shared_ptr<packet_recv_context>& context)
    {
        // 패킷 사이즈 정상적?
        const auto packet_size = flatbuffers::ReadScalar<flatbuffers::uoffset_t>(&prefix) + sizeof(flatbuffers::uoffset_t);
        if (packet_size == 0 || packet_size > net::common::system_config::connection::buffer_size)
        {
            SPDLOG_WARN("invalid packet size : {}. close connection.", packet_size);
            return false;
        }

        // 패킷 사이즈 정상. recv_context 생성
        context = std::make_shared<packet_recv_context>(shared_from_this(), packet_size);

        // payload를 읽기 전에 size-prefix를 먼저 채움
        std::memcpy(context->buffer.data(), &prefix, sizeof(prefix));

        return true;
    }

    bool connection::try_validate_recv_context(packet_recv_context& context)
    {
        // 정상적인 size-prefixed packet?
        flatbuffers::Verifier verifier(context.buffer.data(), context.buffer.size());
        if (!net::protocol::VerifySizePrefixedpacketBuffer(verifier))
        {
            SPDLOG_WARN("invalid packet buffer.");
            return false;
        }

        // 서버에 등록되지 않은 패킷?
        const auto payload_type = context.payload_type();
        packet_handle handle = packet_handle_helper::get(payload_type);
        if (!handle)
        {
            SPDLOG_WARN("recv handler not found. payload_type : {}.", net::protocol::EnumNamepayload_type(payload_type));
            return false;
        }

        // 검증 완료. handler 바인딩
        context.handle = std::move(handle);

        return true;
    }

    bool connection::try_validate_send_context(const packet_send_context& context)
    {
        // 송신 패킷 사이즈 정상적?
        if (context.buffer.size() > net::common::system_config::connection::buffer_size ||
            context.buffer.empty())
        {
            SPDLOG_WARN("invalid packet size : {}.", context.buffer.size());
            return false;
        }

        // 정상적인 size-prefixed packet?
        flatbuffers::Verifier verifier(context.buffer.data(), context.buffer.size());
        if (!net::protocol::VerifySizePrefixedpacketBuffer(verifier))
        {
            SPDLOG_WARN("invalid send packet buffer.");
            return false;
        }

        return true;
    }

    bool connection::enqueue_recv(packet_recv_context&& context)
    {
        // TODO : 서버 - 클라 연결 상태가 안좋음
        // TODO : Late limit, pending 제한 넣어야함

        recv_queue.emplace(std::move(context));
        return true;
    }

    bool connection::enqueue_send(packet_send_context&& context)
    {
        // 서버 - 클라 연결 상태가 안좋음
        const bool is_queue_full = send_queue.size() >= net::common::system_config::connection::queue_size;
        if (is_queue_full)
        {
            SPDLOG_WARN("send queue overflow. close connection.");
            return false;
        }

		send_queue.emplace(std::move(context));
        return true;
    }

} // namespace net::core
