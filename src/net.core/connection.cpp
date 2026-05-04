#include "connection.h"
#include <cstring>
#include <chrono>
#include <array>

namespace net::core
{
    connection::connection(boost::asio::io_context& context, boost::asio::ip::tcp::socket&& client_socket, uint64_t connection_id) :
    context(context),
    strand(context.get_executor()),
    socket(std::move(client_socket)),
    connection_id(connection_id)
    {
        SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::core::connection).name()));
    }

    connection::~connection()
    {
        close();
        SPDLOG_INFO("destroy {} instance.", net::common::demangle(typeid(net::core::connection).name()));
    }

    void connection::close()
    {
        if (!socket.is_open()) return;

        boost::system::error_code error;

        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
        if (error && error != boost::asio::error::not_connected)
        {
            SPDLOG_WARN("connection shutdown failed: {}", error.message());
        }

        error.clear();

        socket.close(error);
        if (error)
        {
            SPDLOG_WARN("connection close failed: {}", error.message());
        }

        error.clear();
    }

} // namespace net::core
