#pragma once
#include "net.common/ts_map.h"
#include "net.common/singleton.h"
#include "net.common/ts_memory_pool.h"
#include "net.core/connection.h"
#include "net.core/packet.h"
#include <moodycamel/blockingconcurrentqueue.h>

namespace net::core
{
	class tcp : public net::common::singleton<tcp>
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

		void init(boost::asio::ip::port_type port);
		void init(const std::string& host, boost::asio::ip::port_type port);
		void start();
		void stop();
		void close();

		bool is_runnable() const { return is_running; }
		moodycamel::BlockingConcurrentQueue<packet_request>& get_requests() { return requests; }

		void async_accept();
		void disconnect(uint32_t connection_id);

	private:

		void on_operation_aborted();
		void on_connection_aborted();
		void on_accept_error();

	private:

		std::atomic<bool> is_running{ false };
		mode mode;
		boost::asio::io_context context;
		std::vector<std::thread> context_workers;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
		boost::asio::ip::tcp::socket socket;
		boost::asio::ip::tcp::endpoint endpoint;
		std::optional<boost::asio::ip::tcp::acceptor> acceptor;
		std::atomic<uint32_t> connection_id_counter { 10000 };
		common::ts_map<uint32_t, std::shared_ptr<connection>> connections;
		moodycamel::BlockingConcurrentQueue<packet_request> requests;
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