#pragma once
#include "net.core/gen/packet.generated.h"
#include "net.common/log.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include <flatbuffers/flatbuffers.h>

// foward declaration
namespace net::core
{
	struct packet_recv_context;
	class connection;
}

// using
namespace net::core
{
	/// <summary>
	/// 수신 패킷을 처리하는 공통 handle 타입
	/// </summary>
	using packet_handle = std::function<void(const packet_recv_context*)>;

	/// <summary>
	/// 구체 payload 타입별 수신 handle 타입
	/// </summary>
	template<typename packet_class_t>
	using packet_handle_t = std::function<void(const packet_recv_context*)>;
}

namespace net::core
{
	/// <summary>
	/// 서버에서 클라로 보낼 패킷 하나의 처리 단위
	/// </summary>
	struct packet_send_context
	{
		/// <summary>
		/// size-prefixed flatbuffers packet buffer
		/// </summary>
		std::vector<uint8_t> buffer;

		/// <summary>
		/// packet_send_context 생성
		/// </summary>
		explicit packet_send_context(std::vector<uint8_t>&& buffer)
		: buffer(std::move(buffer))
		{
		}

		/// <summary>
		/// payload 생성 함수를 사용해 송신 packet 생성
		/// </summary>
		template <typename payload_t, typename create_payload_func_t>
		static packet_send_context create(create_payload_func_t create_payload)
		{
			flatbuffers::FlatBufferBuilder builder;

			const auto payload = create_payload(builder);
			const auto packet = net::protocol::Createpacket
			(
				builder,
				net::protocol::payload_typeTraits<payload_t>::enum_value,
				payload.Union()
			);

			net::protocol::FinishSizePrefixedpacketBuffer(builder, packet);

			std::vector<uint8_t> buffer
			(
				builder.GetBufferPointer(),
				builder.GetBufferPointer() + builder.GetSize()
			);

			return packet_send_context(std::move(buffer));
		}
	};

	/// <summary>
	/// 클라에게서 받은 패킷 하나의 처리 단위 (request)
	/// </summary>
	struct packet_recv_context
	{
		/// <summary>
		/// 패킷을 수신한 connection
		/// </summary>
		std::shared_ptr<connection> owner;

		/// <summary>
		/// size-prefixed flatbuffers packet buffer
		/// </summary>
		std::vector<uint8_t> buffer;

		/// <summary>
		/// 검증 후 바인딩되는 packet handle
		/// </summary>
		packet_handle handle;

		packet_recv_context() = default;

		/// <summary>
		/// 수신 buffer 크기 기반 packet_recv_context 생성
		/// </summary>
		packet_recv_context(std::shared_ptr<connection> owner, uint buffer_size) :
		owner(owner), buffer(buffer_size)
		{
		}

		/// <summary>
		/// 수신 buffer와 handle 기반 packet_recv_context 생성
		/// </summary>
		packet_recv_context(std::shared_ptr<connection> owner, std::vector<uint8_t>&& buffer, packet_handle handle) :
		owner(owner), buffer(std::move(buffer)), handle(std::move(handle))
		{
		}

		/// <summary>
		/// 바인딩된 packet handle 호출
		/// </summary>
		void invoke() const
		{
			if (!handle) return;

			handle(this);
		}

		/// <summary>
		/// flatbuffers packet 루트 반환
		/// </summary>
		const net::protocol::packet* packet() const
		{
			return net::protocol::GetSizePrefixedpacket(buffer.data());
		}

		/// <summary>
		/// payload type 반환
		/// </summary>
		net::protocol::payload_type payload_type() const
		{
			return packet()->payload_type();
		}

		/// <summary>
		/// payload를 구체 타입으로 반환
		/// </summary>
		template <typename payload_class_t>
		const payload_class_t* payload_as() const
		{
			return packet()->payload_as<payload_class_t>();
		}
	};

	/// <summary>
	/// payload type별 수신 handle 등록 및 조회 지원
	/// </summary>
	class packet_handle_helper
	{
	public:

		/// <summary>
		/// payload type에 등록된 handle 반환
		/// </summary>
		static const packet_handle get(net::protocol::payload_type type)
		{
			auto iterator = recv_map.find(type);
			if (iterator == recv_map.end()) return nullptr;

			return iterator->second;
		}

		/// <summary>
		/// payload type과 handle 등록
		/// </summary>
		static bool insert(const net::protocol::payload_type& type, const packet_handle& handle)
		{
			auto [_, is_inserted] = recv_map.emplace(type, handle);
			return is_inserted;
		}

		/// <summary>
		/// 구체적 handle을 감싸는 래퍼 handle 생성
		/// </summary>
		template<typename packet_class_t>
		static packet_handle create_typed_handle(net::protocol::payload_type type, packet_handle_t<packet_class_t> handle)
		{
			// 래퍼 handle
			return [handle](const packet_recv_context* context)
			{
				if (context == nullptr || context->packet() == nullptr)
				{
					SPDLOG_ERROR("Failed to parse packet: null request or packet");
					return;
				}

				const auto* payload = context->payload_as<packet_class_t>();
				if (payload == nullptr)
				{
					SPDLOG_ERROR("Failed to parse payload: {}", net::common::demangle(typeid(packet_class_t).name()));
					return;
				}

				// 구체 handle
				handle(context);
			};
		}

		/// <summary>
		/// 패킷에 대한 handle 등록
		/// </summary>
		template<typename packet_class_t>
		static void register_typed_handle(net::protocol::payload_type type, packet_handle_t<packet_class_t> handle)
		{
			packet_handle new_handle = packet_handle_helper::create_typed_handle<packet_class_t>(type, handle);
			if (!packet_handle_helper::insert(type, new_handle))
			{
				SPDLOG_ERROR("duplicated handle registration. packet type : {}", net::protocol::EnumNamepayload_type(type));
			}
		}

	private:

		/// <summary>
		/// payload type별 수신 handle map
		/// </summary>
		inline static std::unordered_map<net::protocol::payload_type, packet_handle> recv_map;
	};

#define CONCAT_IMPL(x, y) x##y


#define CONCAT(x, y) CONCAT_IMPL(x, y)


#define REGISTER_HANDLE_IMPL(payload_type, payload_class, packet_handle, unique_id) \
static struct CONCAT(auto_registrar_, unique_id) \
{ \
    CONCAT(auto_registrar_, unique_id)() \
    { \
        net::core::packet_handle_helper::register_typed_handle<payload_class>(payload_type, packet_handle); \
    } \
} CONCAT(_auto_registrar_instance_, unique_id);


#define REGISTER_RECV_HANDLE(payload_type, payload_class, packet_handle) \
    REGISTER_HANDLE_IMPL(payload_type, payload_class, packet_handle, __LINE__)
}
