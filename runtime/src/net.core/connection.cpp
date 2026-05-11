#include "connection.h"
#include "net.core/gen/packet.generated.h"
#include <cstring>

namespace net::core
{
    connection::connection(boost::asio::io_context& context, boost::asio::ip::tcp::socket&& client_socket, uint64_t connection_id) :
    strand(context),
    socket(std::move(client_socket)),
    connection_guid(connection_id),
    curr_state(state::connected)
    {
        SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::core::connection).name()));
    }

    connection::~connection()
    {
        boost::system::error_code error;
        socket.close(error);

        SPDLOG_INFO("destroy {} instance.", net::common::demangle(typeid(net::core::connection).name()));
    }

    void connection::async_read()
    {
        async_read_header();
    }

    void connection::close()
    {
        auto self = shared_from_this();

        // close는 여러 경로에서 호출될 수 있으므로 strand에서 한 번만 socket을 닫음
        boost::asio::post(strand, [self]
        {
            if (self->curr_state.load() == state::disconnected) return;

            boost::system::error_code error;
            self->socket.close(error);

            self->curr_state.store(state::disconnected);
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
                if (self->curr_state.load() == state::disconnected)
                {
                    SPDLOG_WARN("connection disconnected.");
                    self->close();
                    return;
                }

                // 패킷 사이즈 정상적?
				const auto packet_size = flatbuffers::ReadScalar<flatbuffers::uoffset_t>(&(self->recv_prefix)) + sizeof(flatbuffers::uoffset_t);
                if (packet_size == 0 || packet_size > net::common::system_config::connection::buffer_size)
                {
                    SPDLOG_WARN("invalid packet size : {}. close connection.", packet_size);
                    self->close();
                    return;
                }

				// 패킷 사이즈 정상. recv_context 생성
				auto context = std::make_shared<packet_recv_context>(self, packet_size);
                std::memcpy(context->buffer.data(), &(self->recv_prefix), sizeof(self->recv_prefix));

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
            boost::asio::buffer(context->buffer.data() + sizeof(recv_prefix), context->buffer.size() - sizeof(recv_prefix)),
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
                if (self->curr_state.load() == state::disconnected)
                {
                    SPDLOG_WARN("connection disconnected.");
                    self->close();
                    return;
                }

                // 정상적인 size-prefixed packet?
                flatbuffers::Verifier verifier(context->buffer.data(), context->buffer.size());
                if (!net::protocol::VerifySizePrefixedpacketBuffer(verifier))
                {
                    SPDLOG_WARN("invalid packet buffer.");
                    self->close();
                    return;
                }

                // 등록된 패킷?
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

    bool connection::enqueue_recv(packet_recv_context&& context)
    {
        // CAUTION : VerifySizePrefixedpacketBuffer()를 먼저 하고 호출 필요
        const auto payload_type = context.payload_type();

        // 등록되지 않은 패킷
        const packet_handle* handle = packet_handle_helper::get(payload_type);
        if (!handle)
        {
            SPDLOG_WARN("recv handler not found. payload_type : {}.", net::protocol::EnumNamepayload_type(payload_type));
            return false;
        }

        // 서버 - 클라 연결 상태가 안좋음
		const bool is_queue_full = recv_queue.size() >= net::common::system_config::connection::queue_size;
        if (is_queue_full)
        {
            SPDLOG_WARN("recv queue overflow. close connection.");
            return false;
        }

        context.handle = *handle;
        recv_queue.emplace(std::move(context));
        return true;
    }

} // namespace net::core
