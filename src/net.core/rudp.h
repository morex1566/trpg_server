#pragma once
#include "net.core/connection.h"
#include "net.common/ts_map.h"
#include "net.common/singleton.h"

namespace net::core
{
	class rudp : public net::common::singleton<rudp>
	{
	public:
		enum class mode
		{
			SERVER,
			CLIENT
		};

		rudp();

		~rudp() noexcept override;

		// 서버 모드로 초기화
		void init(boost::asio::ip::port_type port);

		// 클라이언트 모드로 초기화
		void init(const std::string& host, boost::asio::ip::port_type port);

		// 시작 (서버: accept 루프 / 클라이언트: connect 시도)
		void start();

		// 중지 (io_context stop + worker thread join)
		void stop();

		// 소켓/acceptor 정리
		void close();

		// 현재 실행 중인지 여부
		bool is_runnable() const { return is_running; }

	private:

		// 역할 (서버 / 클라이언트)
		mode mode;

		// 실행 여부
		std::atomic<bool> is_running{ false };

		// dispatch
		boost::asio::io_context context;

		// dispatch thread
		std::vector<std::thread> context_workers;

		// dispatch thread infinite loop
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

		boost::asio::ip::tcp::socket socket;

		// send target ip
		boost::asio::ip::tcp::endpoint endpoint;

		// acceptor
		std::optional<boost::asio::ip::tcp::acceptor> acceptor;

		// 세션 uid 카운터
		std::atomic<uint32_t> connection_id_counter{ 10000 };

		// 세션 관리 맵
		common::ts_map<uint32_t, std::shared_ptr<connection>> connections;
	};

}