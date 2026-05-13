#pragma once
#include "net.core/packet.h"

#include <cstdint>
#include <memory>
#include <queue>
#include <vector>

#include <boost/asio.hpp>
#include <tbb/concurrent_queue.h>

namespace net::core
{
	/// <summary>
	/// 클라이언트 TCP 연결 1개의 송수신 상태 관리
	/// </summary>
	class connection : public std::enable_shared_from_this<connection>
	{
	public:

		/// <summary>
		/// connection 연결 상태
		/// </summary>
		enum class state
		{
			none,
			connected,
			disconnected
		};

		/// <summary>
		/// async_write에서 queue를 flush할 때, 하나로 묶는 용도
		/// </summary>
		struct packet_send_context_batch
		{
			/// <summary>
			/// batch로 묶인 송신 context 목록
			/// </summary>
			std::vector<packet_send_context> contexts;

			/// <summary>
			/// async_write에 전달할 buffer 목록
			/// </summary>
			std::vector<boost::asio::const_buffer> buffers;
		};

	public:

		/// <summary>
		/// connection 인스턴스 생성
		/// </summary>
		connection::connection(boost::asio::io_context& context, boost::asio::ip::tcp::socket&& client_socket, uint64_t connection_id,
			tbb::concurrent_queue<packet_recv_context>& queue);

		/// <summary>
		/// connection 복사 생성 방지
		/// </summary>
		connection(const connection&) = delete;

		/// <summary>
		/// connection 복사 대입 방지
		/// </summary>
		connection& operator=(const connection&) = delete;

		/// <summary>
		/// connection 인스턴스 소멸
		/// </summary>
		~connection();

		/// <summary>
		/// 송신 패킷 enqueue 요청
		/// </summary>
		void send(packet_send_context&& context);

		/// <summary>
		/// 송신 queue flush 시작
		/// </summary>
		void async_write();

		/// <summary>
		/// 수신 read 시작
		/// </summary>
		void async_read();

		/// <summary>
		/// connection 종료
		/// </summary>
		void close();

		state get_state() const { return curr_state; }

		uint64_t get_guid() const { return connection_guid; }

	private:

		/// <summary>
		/// flatbuffers size-prefixed 읽기
		/// </summary>
		void async_read_header();

		/// <summary>
		/// size-prefixed로 union의 payload 읽기
		/// </summary>
		void async_read_payload(std::shared_ptr<packet_recv_context> context);

		void async_write_batch(std::shared_ptr<packet_send_context_batch> batch);

		std::shared_ptr<packet_send_context_batch> create_send_context_batch();

		/// <summary>
		/// 수신 header 단계 : prefix / size 검증 및 context buffer 준비
		/// </summary>
		bool try_validate_recv_context(flatbuffers::uoffset_t prefix, std::shared_ptr<packet_recv_context>& context);

		/// <summary>
		/// 수신 payload 단계 : flatbuffers 검증 및 handle 바인딩
		/// </summary>
		bool try_validate_recv_context(packet_recv_context& context);

		/// <summary>
		/// 송신 enqueue 전 : size / flatbuffers 검증
		/// </summary>
		bool try_validate_send_context(const packet_send_context& context);

		/// <summary>
		/// 수신 packet 처리 queue에 enqueue
		/// </summary>
		bool enqueue_recv(packet_recv_context&& context);

		/// <summary>
		/// 송신 packet 처리 queue에 enqueue
		/// </summary>
		bool enqueue_send(packet_send_context&& context);

	private:

		/// <summary>
		/// connection handler 실행 strand
		/// </summary>
		boost::asio::io_context::strand strand;

		/// <summary>
		/// 클라이언트 TCP socket
		/// </summary>
		boost::asio::ip::tcp::socket socket;

		/// <summary>
		/// async_read_header에서 size-prefixed packet 읽을 때, 패킷 사이즈 담는 변수
		/// </summary>
		flatbuffers::uoffset_t recv_prefix;

		/// <summary>
		/// 검증 완료된 수신 패킷 처리 큐
		/// strand 위에서만 push/pop
		/// </summary>
		tbb::concurrent_queue<packet_recv_context>& recv_queue;

		/// <summary>
		/// strand 위에서만 읽고 쓴다
		/// </summary>
		std::queue<packet_send_context> send_queue;

		/// <summary>
		/// 클라에게 내려주는 서버 측 고유 GUID
		/// </summary>
		uint64_t connection_guid;

		/// <summary>
		/// 현재 connection 상태
		/// </summary>
		state curr_state;

		/// <summary>
		/// async_write 진행 여부
		/// </summary>
		bool is_writing;
	};
}
