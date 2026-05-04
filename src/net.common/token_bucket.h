#pragma once
#include <atomic>
#include <cstdint>

namespace net::common
{
    class token_bucket
    {
    public:
        token_bucket(uint64_t capacity, double refill_interval_ms);

        bool consume(uint64_t amount = 1);

    private:

        struct alignas(8) token_state
        {
            uint64_t tokens;
            uint64_t last_ms;
        };

        /// <summary>
        /// 버킷 최대 사이즈
        /// </summary>
        uint64_t capacity;

        /// <summary>
        /// 토큰 1개가 회복되는 간격(ms)
        /// </summary>
        double refill_interval_ms;

        /// <summary>
        /// 클라가 접속하면 이 토큰이 1개 소모
        /// </summary>
        std::atomic<token_state> state;
    };
}
