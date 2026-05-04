#pragma once
#include "net.common/global_singleton.h"
#include "net.common/system_config.h"
#include "net.common/token_bucket.h"
#include "net.common/connection_id_generator.h"
#include "net.common/log.h"
#include "net.core/connection.h"
#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <tbb/concurrent_hash_map.h>

namespace net::core
{
	class tcp_server : public net::common::global_singleton<tcp_server>
	{
	public:

		enum class state
		{
			stopped,
			running
		};

	public:

		tcp_server();
		~tcp_server() noexcept override;

		// 서버 주소 초기화
		void init(boost::asio::ip::port_type port);

		// 클라이언트 접속 받기 시작
		void async_accept();

		void close();

		state get_state() const { return current_state.load(); }

	private:

		// asio 디스패치 받을 스레드 시작
		void start_workers();

		// asio 디스패치 받을 스레드 종료
		void stop_workers();

		void post_accept();

		std::string get_remote_address(boost::asio::ip::tcp::socket& client_socket);

	private:

		// 서버 실행 상태
		std::atomic<state> current_state { state::stopped };

		// OS 디스패치 처리
		boost::asio::io_context context;

		// OS 디스패치를 여러 스레드가 처리하기 위해
		std::vector<std::thread> context_workers;

		// io_context dispatch가 계속되도록
		std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

		// Rate Limit용
		net::common::token_bucket accept_token_bucket;

		// 이 서버 주소
		boost::asio::ip::tcp::endpoint endpoint;

		// accept용
		std::optional<boost::asio::ip::tcp::acceptor> acceptor;

		// 연결된 클라이언트 목록
		tbb::concurrent_hash_map<net::common::connection_id, std::shared_ptr<connection>> connections;
	};
}
