#pragma once
#include "net.common/global_singleton.h"
#include "net.common/system_config.h"
#include "net.common/token_bucket.h"
#include "net.common/connection_id_generator.h"
#include "net.common/log.h"
#include "net.core/connection.h"
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <tbb/concurrent_hash_map.h>


namespace net::core
{
	class tcp : public net::common::global_singleton<tcp>
	{
	public:

		enum class mode 
		{ 
			SERVER, 
			CLIENT 
		};

	public:

		tcp();
		~tcp() noexcept override;

		/// <summary>
		/// 서버 모드로 초기화
		/// </summary>
		void init(boost::asio::ip::port_type port);

		/// <summary>
		/// 클라이언트 모드로 초기화
		/// </summary>
		void init(const std::string& host, boost::asio::ip::port_type port);

		/// <summary>
		/// 서버 모드로 클라 접속 받기
		/// </summary>
		void async_accept();

		/// <summary>
		/// 클라이언트 모드로 서버에 연결
		/// </summary>
		void async_connect();

		void close();

		bool is_running() const { return b_running; }

	private:

		void start_workers();

		void stop_workers();

		void post_accept();

		std::string remote_address_or_unknown(boost::asio::ip::tcp::socket& client_socket);

	private:

		/// <summary>
		/// 서버가 클라이언트에게 열려있음?
		/// </summary>
		std::atomic<bool> b_running { false };

		mode mode;

		/// <summary>
		/// OS 디스패치 처리
		/// </summary>
		boost::asio::io_context context;

		/// <summary>
		/// OS 디스패치 처리를 여러 스레드가 처리하기 위해
		/// </summary>
		std::vector<std::thread> context_workers;

		/// <summary>
		/// io_context dispatch가 계속되도록
		/// </summary>
		std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

		/// <summary>
		/// 이 tcp socket
		/// </summary>
		boost::asio::ip::tcp::socket socket;

		/// <summary>
		/// Rate Limit용
		/// </summary>
		net::common::token_bucket accept_token_bucket;

		/// <summary>
		/// 이 서버 주소
		/// </summary>
		boost::asio::ip::tcp::endpoint endpoint;

		/// <summary>
		/// accept용
		/// </summary>
		std::optional<boost::asio::ip::tcp::acceptor> acceptor;

		/// <summary>
		/// 연결된 클라이언트 목록
		/// </summary>
		tbb::concurrent_hash_map<net::common::connection_id, std::shared_ptr<connection>> connections;
	};
}
