#pragma once
#include "net.common/system_config.h"
#include "net.common/time.h"
#include "net.common/connection_id_generator.h"
#include "net.common/log.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <cstdlib>
#include <optional>
#include <atomic>
#include <tbb/concurrent_queue.h>

namespace net::core
{
	class connection : public std::enable_shared_from_this<connection>
	{
	public:

		connection(boost::asio::io_context& context, boost::asio::ip::tcp::socket&& client_socket, uint64_t connection_id);
		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;
		~connection();


		void close();


	private:

		boost::asio::io_context& context;

		boost::asio::strand<boost::asio::io_context::executor_type> strand;

		// 클라이언트 소켓
		boost::asio::ip::tcp::socket socket;

		// 일회성 고유 id
		uint64_t connection_id;
	};
}
