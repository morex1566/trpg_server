#pragma once
#include "net.core/packet.h"
#include "net.common/system_config.h"
#include "net.common/ts_memory_pool.h"
#include <boost/asio.hpp>
#include <moodycamel/blockingconcurrentqueue.h>
#include <functional>

namespace net::core
{
	class connection : public std::enable_shared_from_this<connection>
	{
	public:

		connection
		(
			boost::asio::io_context& context,
			boost::asio::ip::tcp::socket&& client_socket,
			uint32_t connection_id,
			std::function<void(uint32_t)> on_disconnected,
			moodycamel::BlockingConcurrentQueue<packet_request>& requests
		);

		connection(const connection&) = delete;

		connection& operator=(const connection&) = delete;

		~connection();

		void close();

		void start();

		void stop();

	private:

		// packet header 읽기
		void async_read_header();

		// packet payload 읽기
		void async_read_payload(std::shared_ptr<uint8_t> packet_owner, uint16_t payload_size);

		void on_operation_aborted();

		void on_connection_aborted();

		void on_read_error();

	private:

		std::atomic<bool> is_running = false;

		std::atomic<bool> is_writing = false;

		std::atomic<bool> is_reading = false;

		boost::asio::io_context& context;

		boost::asio::strand<boost::asio::io_context::executor_type> strand;

		// 클라이언트 소켓
		boost::asio::ip::tcp::socket socket;

		// 세션의 고유 id
		uint32_t connection_id;

		std::function<void(uint32_t)> on_disconnected;

		// 고정 크기 헤더 임시 버퍼 (20 bytes)
		uint8_t header_read_buffer[sizeof(net::protocol::packet_header)];

		// 디스패처가 읽는 부분
		// TODO : 시퀀스 -> priority queue로 구현?
		moodycamel::BlockingConcurrentQueue<packet_request>& requests;
	};
}

// err						: boost::system::error_code
// on_operation_aborted     : 서버 소켓 닫힘 등으로 인한 작업 취소 시 실행할 구문
// on_conn_aborted			: 클라이언트가 일방적으로 연결을 끊었을 때 실행할 구문 (주로 다음 accept 재등록)
// on_error					: 기타 심각한 에러 발생 시 실행할 구문
#define CHECK_READ_ERROR(err, on_operation_aborted, on_conn_aborted, on_error) \
    do { \
        if (err) { \
            if (err == boost::asio::error::operation_aborted) { \
                on_operation_aborted; \
            } \
            else \
			if (err == boost::asio::error::connection_aborted) { \
                on_conn_aborted; \
            } \
            else { \
                on_error; \
            } \
            return; /* 핸들러 실행 종료 */ \
        } \
    } while (0)