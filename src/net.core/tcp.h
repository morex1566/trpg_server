#pragma once
#include "net.common/ts_map.h"
#include "net.common/singleton.h"
#include "net.common/ts_memory_pool.h"
#include "net.core/connection.h"
#include "net.packet/packet.h"
#include <moodycamel/blockingconcurrentqueue.h>

namespace net::core
{
	// TCP 서버 / 클라이언트 통합 클래스
	// - 서버 모드: tcp(port)
	// - 클라이언트 모드: tcp(host, port)
	class tcp : public net::common::singleton<tcp>
	{
	public:

		enum class mode 
		{ 
			SERVER, 
			CLIENT 
		};

		tcp();
		~tcp() noexcept override;

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

		moodycamel::BlockingConcurrentQueue<net::packet::packet_request>& get_requests() { return requests; }

		// 서버 모드
		void async_accept();

		void disconnect(uint32_t connection_id);

	private:

		void on_operation_aborted();

		void on_connection_aborted();

		void on_accept_error();

	private:

		// 실행 여부
		std::atomic<bool> is_running{ false };

		// 역할 (서버 / 클라이언트)
		mode mode;

		// IOCP
		boost::asio::io_context context;

		// IOCP 스레드
		std::vector<std::thread> context_workers;

		// IOCP 작업 없을 때 꺼지지 않도록
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

		// TCP 소켓 (클라이언트 모드: connect 용 / 서버 모드: accept 임시 socket)
		boost::asio::ip::tcp::socket socket;

		// 접속 대상 엔드포인트
		boost::asio::ip::tcp::endpoint endpoint;

		// acceptor
		std::optional<boost::asio::ip::tcp::acceptor> acceptor;

		// 세션 uid 카운터
		std::atomic<uint32_t> connection_id_counter { 10000 };

		// 세션 관리 맵
		common::ts_map<uint32_t, std::shared_ptr<connection>> connections;

		// 디스패처가 읽는 부분
		// TODO : 시퀀스 -> priority queue로 구현?
		moodycamel::BlockingConcurrentQueue<net::packet::packet_request> requests;
	};
}

// err			   : boost::system::error_code
// on_aborted      : 서버 소켓 닫힘 등으로 인한 작업 취소 시 실행할 구문
// on_conn_aborted : 클라이언트가 일방적으로 연결을 끊었을 때 실행할 구문 (주로 다음 accept 재등록)
// on_error        : 기타 심각한 에러 발생 시 실행할 구문
#define CHECK_ACCEPT_ERROR(err, on_aborted, on_conn_aborted, on_error) \
    do { \
        if (err) { \
            if (err == boost::asio::error::operation_aborted) { \
                on_aborted; \
            } \
            else if (err == boost::asio::error::connection_aborted) { \
                on_conn_aborted; \
            } \
            else { \
                on_error; \
            } \
            return; /* 핸들러 실행 종료 */ \
        } \
    } while (0)