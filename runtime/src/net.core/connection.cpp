#include "connection.h"
#include <cstring>
#include <chrono>
#include <array>

namespace net::core
{
    connection::connection(boost::asio::io_context& context, boost::asio::ip::tcp::socket&& client_socket, uint64_t connection_id) :
    context(context),
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
        // connected 상태가 아님.
        state expected = state::connected;
        if (!current_state.compare_exchange_strong(expected, state::disconnected)) return;

        boost::system::error_code error;
        socket.close(error);
    }

} // namespace net::core
