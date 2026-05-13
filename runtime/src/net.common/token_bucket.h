#pragma once
#include <atomic>
#include <cstdint>

namespace net::common
{
    /// <summary>
    /// CAS 기반 token bucket rate limiter
    /// </summary>
    class token_bucket
    {
    public:

        /// <summary>
        /// token bucket 생성
        /// </summary>
        token_bucket(uint64_t capacity, double refill_interval_ms);

        /// <summary>
        /// 지정한 token 수만큼 소모 시도
        /// </summary>
        bool consume(uint64_t amount = 1);

    private:

        /// <summary>
        /// atomic으로 갱신되는 token 상태
        /// </summary>
        struct alignas(8) token_state
        {
            /// <summary>
            /// 현재 token 수
            /// </summary>
            uint64_t tokens;

            /// <summary>
            /// 마지막 token 갱신 시간(ms)
            /// </summary>
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
