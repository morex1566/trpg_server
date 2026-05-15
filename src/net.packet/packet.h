#pragma once
#include "net.packet/packet_generated.h"
#include "net.common/system_config.h"
#include "net.common/log.h"
#include <unordered_map>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>

namespace net::packet
{
    struct packet_request;
    using packet_handler = std::function<void(const packet_request*)>;

    struct packet_request
    {
        uint32_t session_id;
        uint16_t packet_id;
        std::shared_ptr<uint8_t> packet_owner;
    };

    class packet_dispatcher
    {
    public:

        static void map(unsigned short packet_id, packet_handler handler)
        {
            auto& handlers = get_handlers();
            handlers[packet_id] = std::move(handler);
        }

        static void dispatch(const packet_request* request)
        {
            auto& handlers = get_handlers();
            if (auto it = handlers.find(request->packet_id); it != handlers.end())
            {
                it->second(request);
            }
            else
            {
                SPDLOG_ERROR("Unhandled packet ID: {}", request->packet_id);
            }
        }

    public:

        static const int max_packet_dispatch_threshold = 1024;

    private:

        // C++ Static Initialization Order Fiasco 방지를 위한 싱글톤 방식 유지
        static std::unordered_map<unsigned short, packet_handler>& get_handlers()
        {
            static std::unordered_map<unsigned short, packet_handler> handlers;
            return handlers;
        }

    };

    // --- 타입 추론을 위한 헬퍼 함수 ---
    // 핸들러 함수의 매개변수 타입(T)을 자동으로 추론합니다.
    template<typename packet_type>
    static void register_typed_handler(unsigned short packet_id, void(*handler_func)(const packet_request*, const packet_type*))
    {
        packet_dispatcher::map(packet_id, [handler_func, packet_id](const packet_request* request)
        {
            const uint8_t* payload_start_offset = request->packet_owner.get() + sizeof(net::protocol::packet_header);
            const size_t payload_size = reinterpret_cast<const net::protocol::packet_header*>(request->packet_owner.get())->payload_size();

            // FlatBuffers Verifier를 사용하여 데이터 정합성 검증 (보안 및 안정성 강화)
            flatbuffers::Verifier verifier(payload_start_offset, payload_size);
            if (!verifier.VerifyBuffer<packet_type>(nullptr)) 
            {
                SPDLOG_ERROR("FlatBuffers verification failed for packet_id: {}", packet_id);
                return;
            }

            const auto* data = flatbuffers::GetRoot<packet_type>(payload_start_offset);
            if (data == nullptr)
            {
                SPDLOG_ERROR("Failed to parse flatbuffers for packet_id: {}", packet_id);
                return;
            }

            handler_func(request, data);
        });
    }
}

// 매크로 전개 순서를 제어하는 CONCAT 구현을 사용하여 __LINE__을 고유 이름으로 변환
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define REGISTER_HANDLER_IMPL(packet_id, handler_func, unique_id) \
static struct CONCAT(auto_registrar_, unique_id) \
{ \
    CONCAT(auto_registrar_, unique_id)() \
    { \
        net::packet::register_typed_handler(packet_id, handler_func); \
    } \
} CONCAT(_auto_registrar_instance_, unique_id);

#define REGISTER_HANDLER(packet_id, handler_func) \
    REGISTER_HANDLER_IMPL(packet_id, handler_func, __LINE__)
