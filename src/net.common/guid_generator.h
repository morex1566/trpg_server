#pragma once
#include "time.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <functional>

namespace net::common
{
    using guid = uint64_t;
}

namespace net::common
{
    class guid_generator
    {
    public:

        guid_generator() = delete;

        static guid generate();

    private:

        static uint64_t current_millis();
        static uint64_t resolve_node_id();

    private:

        static constexpr uint64_t SEQ_BITS = 12;
        static constexpr uint64_t NODE_BITS = 10;
        static constexpr uint64_t NODE_SHIFT = SEQ_BITS;
        static constexpr uint64_t TS_SHIFT = NODE_BITS + SEQ_BITS;
        static constexpr uint64_t SEQ_MASK = (1ULL << SEQ_BITS) - 1;
        static constexpr uint64_t NODE_MASK = (1ULL << NODE_BITS) - 1;

        static const uint64_t node_id;
        static std::atomic<uint64_t> state;
    };
}
