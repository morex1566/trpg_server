#pragma once
#include "net.common/global_singleton.h"
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
	class tcp_client : public net::common::global_singleton<tcp_client>
	{
	public:

		enum class state
		{
			disconnected,
			connecting,
			connected
		};

	public:

		tcp_client();
		~tcp_client() noexcept override;

		// 서버 주소 초기화
		void init(const std::string& host, boost::asio::ip::port_type port);

		// 서버에 연결 시작
		void async_connect();

		void close();

		state get_state() const { return current_state.load(); }

	private:

		// asio 디스패치 받을 스레드 시작
		void create_workers();

		// asio 디스패치 받을 스레드 종료
		void delete_workers();

		void post_resolve();

		void post_connect(boost::asio::ip::tcp::resolver::results_type results);

	private:

		// 클라이언트 연결 상태
		std::atomic<state> current_state { state::disconnected };

		// OS 디스패치 처리
		boost::asio::io_context context;

		// OS 디스패치를 여러 스레드가 처리하기 위해
		std::vector<std::thread> context_workers;

		// io_context dispatch가 계속되도록
		std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

		// 서버 주소 DNS resolve용
		boost::asio::ip::tcp::resolver resolver;

		// 이 tcp socket
		boost::asio::ip::tcp::socket socket;

		// 연결할 서버 host
		std::string host;

		// 연결할 서버 port
		boost::asio::ip::port_type port { 0 };

		// 서버 연결 목록
		tbb::concurrent_hash_map<net::common::connection_id, std::shared_ptr<connection>> connections;
	};
}
