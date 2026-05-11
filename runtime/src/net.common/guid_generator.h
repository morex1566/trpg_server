#pragma once

#include <atomic>
#include <cstdint>
#include <random>
#include <stdexcept>

namespace net::common
{
    class guid_generator
    {
    public:

        static std::uint64_t generate()
        {
            static const std::uint64_t server_salt = create_server_salt();
            static std::atomic<std::uint64_t> sequence{ 0 };

			const std::uint64_t current_sequence = sequence.fetch_add(1, std::memory_order_relaxed);

            return (server_salt << sequence_bits) | current_sequence;
        }

    private:

        static std::uint64_t create_server_salt()
        {
            std::random_device random_device;

            return static_cast<std::uint64_t>(random_device()) & 0xffffull;
        }

    private:

        static constexpr std::uint64_t sequence_bits = 48;
        static constexpr std::uint64_t sequence_mask = (1ull << sequence_bits) - 1ull;
    };
}
