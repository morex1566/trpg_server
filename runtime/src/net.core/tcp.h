#pragma once
#include "net.core/connection.h"
#include "net.common/global_singleton.h"
#include "net.common/token_bucket.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <tbb/concurrent_queue.h>

namespace net::core
{
	/// <summary>
	/// TCP 서버 accept와 connection 목록 관리
	/// </summary>
	class tcp : public net::common::global_singleton<tcp>
	{
	public:

		/// <summary>
		/// tcp 서버 실행 상태
		/// </summary>
		enum class state
		{
			stopped,
			running
		};

	public:

		/// <summary>
		/// tcp 인스턴스 생성
		/// </summary>
		tcp();

		/// <summary>
		/// tcp 인스턴스 소멸
		/// </summary>
		~tcp() noexcept override;

		/// <summary>
		/// 서버 주소 초기화
		/// </summary>
		void init(boost::asio::ip::port_type port);

		/// <summary>
		/// connection애 dispatch 및 writing을 명령
		/// </summary>
		void update();

		/// <summary>
		/// 클라이언트 접속 받기 시작
		/// </summary>
		void async_accept();

		/// <summary>
		/// write tick-rate마다 모든 connection에 async_write 명령
		/// </summary>
		void async_write();

		/// <summary>
		/// tcp 서버 종료
		/// </summary>
		void close();

		state get_state() const { return current_state.load(); }

		tbb::concurrent_queue<packet_recv_context>& get_recv_queue() { return recv_queue; }

	private:

		/// <summary>
		/// asio 디스패치 받을 스레드 시작
		/// CAUTION : 비동기적 호출X
		/// </summary>
		void create_workers();

		/// <summary>
		/// asio 디스패치 받을 스레드 종료
		/// CAUTION : 비동기적 호출X
		/// </summary>
		void delete_workers();

		/// <summary>
		/// 다음 accept 요청 등록
		/// </summary>
		void async_accept_next();

		/// <summary>
		/// 클라이언트 socket의 remote address 반환
		/// </summary>
		std::string get_remote_address(boost::asio::ip::tcp::socket& client_socket);

	private:

		/// <summary>
		/// 서버 실행 상태
		/// </summary>
		std::atomic<state> current_state { state::stopped };

		/// <summary>
		/// OS 디스패치 처리
		/// </summary>
		boost::asio::io_context context;

		/// <summary>
		/// tcp accept / close / write 직렬화용
		/// </summary>
		boost::asio::io_context::strand strand;

		/// <summary>
		/// write tick-rate용 타이머
		/// </summary>
		boost::asio::steady_timer write_timer;

		/// <summary>
		/// OS 디스패치를 여러 스레드가 처리하기 위해
		/// </summary>
		std::vector<std::thread> context_workers;

		/// <summary>
		/// io_context dispatch가 계속되도록
		/// </summary>
		std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

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
		/// connection_id_generator가 uint64_t를 반환하므로 key도 같은 크기로 유지
		/// </summary>
		std::unordered_map<uint64_t, std::shared_ptr<connection>> connections;

		/// <summary>
		/// 검증 완료된 수신 패킷 처리 큐
		/// </summary>
		tbb::concurrent_queue<packet_recv_context> recv_queue;
	};
}
