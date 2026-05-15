#include "pch.h"
#include "connection.h"

net::core::connection::connection
(
    boost::asio::io_context& context,
    boost::asio::ip::tcp::socket&& client_socket,
    uint32_t connection_id,
    std::function<void(uint32_t)> on_disconnected,
    moodycamel::BlockingConcurrentQueue<net::packet::packet_request>& requests
) : context(context),
    strand(context.get_executor()),
    socket(std::move(client_socket)),
    connection_id(connection_id),
    on_disconnected(on_disconnected),
    requests(requests)
{
    SPDLOG_INFO("client connected. {}", connection_id);
}

net::core::connection::~connection()
{
    close();
}

void net::core::connection::close()
{
    if (socket.is_open())
    {
        socket.close();
    }
}

void net::core::connection::start()
{
    is_running = true;

    async_read_header();
}

void net::core::connection::stop()
{
    is_running = false;
}

void net::core::connection::async_read_header()
{
    auto self = shared_from_this();

    boost::asio::async_read(socket, boost::asio::buffer(&header_read_buffer, sizeof(net::protocol::packet_header)), 
        boost::asio::bind_executor(strand, [this, self](boost::system::error_code error, size_t len)
        {
            CHECK_READ_ERROR(error, on_operation_aborted(), on_connection_aborted(), on_read_error());
            CHECK_RETURN_VOID(!is_running);

            const auto* header = reinterpret_cast<const net::protocol::packet_header*>(header_read_buffer);
            uint16_t payload_size = header->payload_size();
    
            // 패킷 공간 풀링
            auto packet_owner = net::common::ts_memory_pool::get_instance().rent(sizeof(net::protocol::packet_header) + payload_size);
            if (!packet_owner)
            {
                SPDLOG_ERROR("packet_owner is null. Disconnecting session_id: {}", connection_id);
                if (on_disconnected) on_disconnected(connection_id);
                stop();
                return;
            }

            // 헤더 복사 (Alignment 안전성을 위해 memcpy 사용 유지)
            memcpy(packet_owner.get(), header_read_buffer, sizeof(net::protocol::packet_header));

            // 페이로드 읽기로 전환
            async_read_payload(packet_owner, payload_size);
        }));
}

void net::core::connection::async_read_payload(std::shared_ptr<uint8_t> packet_owner, uint16_t payload_size)
{
    auto self = shared_from_this();

    // 헤더 이후 위치부터 페이로드 수신
    boost::asio::async_read(socket, boost::asio::buffer(packet_owner.get() + sizeof(net::protocol::packet_header), payload_size),
        boost::asio::bind_executor(strand, [this, self, packet_owner](boost::system::error_code error, size_t len)
        {
            CHECK_READ_ERROR(error, on_operation_aborted(), on_connection_aborted(), on_read_error());
            CHECK_RETURN_VOID(!is_running);

            const auto* header = reinterpret_cast<const net::protocol::packet_header*>(packet_owner.get());

            // 리퀘스트 등록
            requests.enqueue(net::packet::packet_request{ connection_id, header->type_id(), packet_owner });

            async_read_header();
        }));
}

void net::core::connection::on_operation_aborted()
{
    SPDLOG_WARN("connection lost. {}", connection_id);

    stop();
    close();
    if (on_disconnected) on_disconnected(connection_id);
}

void net::core::connection::on_connection_aborted()
{
    SPDLOG_WARN("connection lost. {}", connection_id);

    stop();
    close();
    if (on_disconnected) on_disconnected(connection_id);
}

void net::core::connection::on_read_error()
{
    SPDLOG_WARN("connection lost. {}", connection_id);

    stop();
    close();
    if (on_disconnected) on_disconnected(connection_id);
}