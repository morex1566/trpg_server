#include "guid_generator.h"

namespace net::common
{
    const uint64_t guid_generator::node_id = resolve_node_id();
    std::atomic<uint64_t> guid_generator::state{ 0 };

    guid guid_generator::generate()
    {
        uint64_t now_ms = current_millis();
        uint64_t expected = state.load(std::memory_order_relaxed);
        uint64_t desired = 0;
        uint64_t seq = 0;

        do
        {
            const uint64_t last_ms = expected >> SEQ_BITS;
            seq = expected & SEQ_MASK;

            if (now_ms == last_ms)
            {
                seq = (seq + 1) & SEQ_MASK;
                if (seq == 0)
                {
                    do { now_ms = current_millis(); } while (now_ms <= last_ms);
                    continue;
                }
            }
            else if (now_ms > last_ms)
            {
                seq = 0;
            }
            else
            {
                now_ms = last_ms;
                seq = (seq + 1) & SEQ_MASK;
            }

            desired = (now_ms << SEQ_BITS) | seq;
        } while (!state.compare_exchange_weak(
            expected, desired,
            std::memory_order_relaxed,
            std::memory_order_relaxed));

        return (now_ms << TS_SHIFT) | (node_id << NODE_SHIFT) | seq;
    }

    uint64_t guid_generator::current_millis()
    {
        return net::common::time::get_instance().timestamp(net::common::time::time_unit_type::millisecond);
    }

    uint64_t guid_generator::resolve_node_id()
    {
        if (const char* env = std::getenv("GUID_NODE_ID"))
        {
            return static_cast<uint64_t>(std::strtoull(env, nullptr, 10)) & NODE_MASK;
        }

#ifdef _WIN32
        const char* host = std::getenv("COMPUTERNAME");
#else
        const char* host = std::getenv("HOSTNAME");
#endif
        const std::string basis = host ? host : "unknown-host";
        return std::hash<std::string>{}(basis) & NODE_MASK;
    }
}
