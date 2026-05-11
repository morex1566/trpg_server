#pragma once
#include "net.core/gen/packet.generated.h"
#include "net.common/log.h"
#include <flatbuffers/flatbuffers.h>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

// foward declaration
namespace net::core
{
	struct packet_recv_context;
	class connection;
}

// using
namespace net::core
{
	using packet_handle = std::function<void(const packet_recv_context*)>;

	template<typename packet_class_t>
	using packet_handle_t = std::function<void(const packet_recv_context*, const packet_class_t*)>;
}

namespace net::core
{
	// 클라에게서 받은 패킷 하나의 처리 단위 (request)
	struct packet_recv_context
	{
		std::shared_ptr<connection> owner;
		std::vector<uint8_t> buffer;
		packet_handle handle;

		packet_recv_context(std::shared_ptr<connection> owner, uint buffer_size)
		: owner(owner), buffer(buffer_size)
		{
		}

		const net::protocol::packet* packet() const
		{
			return net::protocol::GetSizePrefixedpacket(buffer.data());
		}

		net::protocol::payload_type payload_type() const
		{
			return packet()->payload_type();
		}

		template <typename payload_class_t>
		const payload_class_t* payload_as() const
		{
			return packet()->payload_as<payload_class_t>();
		}
	};

	class packet_handle_helper
	{
	public:

		static const packet_handle* get(net::protocol::payload_type type)
		{
			auto iterator = recv_map.find(type);
			if (iterator == recv_map.end()) return nullptr;

			return &iterator->second;
		}

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

				const auto* payload = context->packet()->payload_as<packet_class_t>();
				if (payload == nullptr)
				{
					SPDLOG_ERROR("Failed to parse payload: {}", net::common::demangle(typeid(packet_class_t).name()));
					return;
				}

				// 구체 handle
				handle(context, payload);
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
