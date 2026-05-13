#pragma once

#include <atomic>
#include <cstdint>
#include <random>

namespace net::common
{
    /// <summary>
    /// 서버 salt와 sequence를 조합해 guid를 생성
    /// </summary>
    class guid_generator
    {
    public:

        /// <summary>
        /// 다음 guid 생성
        /// </summary>
        static std::uint64_t generate()
        {
            static const std::uint64_t server_salt = create_server_salt();
            static std::atomic<std::uint64_t> sequence{ 0 };

			const std::uint64_t current_sequence = sequence.fetch_add(1, std::memory_order_relaxed);

            return (server_salt << sequence_bits) | current_sequence;
        }

    private:

        /// <summary>
        /// 서버 프로세스별 salt 생성
        /// </summary>
        static std::uint64_t create_server_salt()
        {
            std::random_device random_device;

            return static_cast<std::uint64_t>(random_device()) & 0xffffull;
        }

    private:

        /// <summary>
        /// sequence에 사용할 bit 수
        /// </summary>
        static constexpr std::uint64_t sequence_bits = 48;

        /// <summary>
        /// sequence bit mask
        /// </summary>
        static constexpr std::uint64_t sequence_mask = (1ull << sequence_bits) - 1ull;
    };
}
