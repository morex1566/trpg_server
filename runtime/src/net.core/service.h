#pragma once
#include "net.core/connection.h"
#include "net.core/packet.h"
#include "net.common/log.h"

#include <cstdint>
#include <string>
#include <utility>

// 클라 요청 처리
namespace net::core
{
	/// <summary>
	/// guid 요청 처리
	/// </summary>
	inline void guid_recv(const packet_recv_context* context)
	{
		const uint64_t guid = context->owner->get_guid();

		// TEMP : 로그로 guid 확인
		SPDLOG_INFO
		(
			"guid response. type : {}, guid : {}.",
			static_cast<int32_t>(context->payload_type()),
			guid
		);

		packet_send_context send_context = packet_send_context::create<net::protocol::guid_recv>
		(
			[guid](flatbuffers::FlatBufferBuilder& builder)
			{
				return net::protocol::Createguid_recv(builder, guid);
			}
		);

		context->owner->send(std::move(send_context));
	}
	REGISTER_RECV_HANDLE(net::protocol::payload_type::payload_type_guid_recv, net::protocol::guid_recv, guid_recv)

	/// <summary>
	/// chat 요청 처리
	/// </summary>
	inline void chat_recv(const packet_recv_context* context)
	{
		const auto* packet = context->payload_as<net::protocol::chat_recv>();
		const auto message = packet->message() ? packet->message()->str() : "";

		// TEMP : 로그로 메시지 확인
		SPDLOG_INFO
		(
			"chat response. type : {}, guid : {}, timestamp_ms : {}, message : {}.",
			static_cast<int32_t>(packet->type()),
			packet->from_guid(),
			packet->timestamp_ms(),
			message
		);
	}
	REGISTER_RECV_HANDLE(net::protocol::payload_type::payload_type_chat_recv, net::protocol::chat_recv, chat_recv)
}
