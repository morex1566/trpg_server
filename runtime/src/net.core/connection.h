#pragma once
#include "net.common/system_config.h"
#include "net.common/log.h"
#include "net.core/packet.h"
#include <atomic>
#include <array>
#include <memory>
#include <queue>

#include <boost/asio.hpp>
#include <flatbuffers/flatbuffers.h>

namespace net::core
{
	class connection : public std::enable_shared_from_this<connection>
	{
	public:

		enum class state
		{
			none,
			connected,
			disconnected
		};

	public:

		connection(boost::asio::io_context& context, boost::asio::ip::tcp::socket&& client_socket, uint64_t connection_guid);
		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;
		~connection();

		void async_read();

		void close();

		bool is_disconnected() const { return curr_state.load() == state::disconnected; }

	private:

		/// <summary>
		/// flatbuffers size-prefixed 읽기
		/// </summary>
		void async_read_header();

		/// <summary>
		/// size-prefixed로 union의 payload 읽기
		/// </summary>
		void async_read_payload(std::shared_ptr<packet_recv_context> context);

		bool enqueue_recv(packet_recv_context&& context);

	private:

		boost::asio::io_context::strand strand;

		boost::asio::ip::tcp::socket socket;

		/// <summary>
		/// async_read_header에서 size-prefixed packet 읽을 때, 패킷 사이즈 담는 변수
		/// </summary>
		flatbuffers::uoffset_t recv_prefix;

		// 검증 완료된 수신 패킷 처리 큐
		// strand 위에서만 push/pop
		std::queue<packet_recv_context> recv_queue;

		// 클라에게 내려주는 서버 측 고유 GUID
		uint64_t connection_guid;

		std::atomic<state> curr_state;
	};
}
