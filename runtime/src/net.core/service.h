#pragma once
#include "net.core/connection.h"
#include "net.common/time.h"
#include "net.core/packet.h"

// 클라 요청 처리
namespace net::core
{
	inline void guid_response(const packet_recv_context* recv_context, const net::protocol::guid_send_response* packet)
	{

	}
	REGISTER_RECV_HANDLE(net::protocol::payload_type_guid_send_response, net::protocol::guid_send_response, guid_response)

	inline void chat_response(const packet_recv_context* recv_context, const net::protocol::chat_send_response* packet)
	{

	}
	REGISTER_RECV_HANDLE(net::protocol::payload_type_chat_send_response, net::protocol::chat_send_response, chat_response)
}
