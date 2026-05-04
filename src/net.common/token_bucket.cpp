#include "token_bucket.h"
#include "net.common/time.h"
#include <algorithm>
#include <stdexcept>

namespace net::common
{
    token_bucket::token_bucket(uint64_t capacity, double refill_interval_ms) :
        capacity(capacity),
        refill_interval_ms(refill_interval_ms)
    {
        state.store({ capacity, net::common::time::get_instance().timestamp() }, std::memory_order_relaxed);
    }

    bool token_bucket::consume(uint64_t amount)
    {
        token_state expected = state.load(std::memory_order_acquire);
        token_state desire;

        do
        {
            const uint64_t current_ms = net::common::time::get_instance().timestamp();
            const uint64_t elapsed_ms = current_ms - expected.last_ms;

            // 지난 시간만큼 token 추가
            const uint64_t current_tokens = std::min(capacity, expected.tokens);
            const uint64_t generated = static_cast<uint64_t>(elapsed_ms / refill_interval_ms);
            const uint64_t available_tokens = (generated >= capacity - current_tokens) ? capacity : current_tokens + generated;

            // 토큰 없다...
            // 클라 연결 거부
            if (available_tokens < amount)
            {
                return false;
            }

            // 토큰 있다...
            // 토큰 소모하고 정보 최신화
            desire.tokens = available_tokens - amount;
            desire.last_ms = (generated > 0) ? expected.last_ms + static_cast<uint64_t>(generated * refill_interval_ms) : expected.last_ms;
        } 
        while (!state.compare_exchange_weak(expected, desire, std::memory_order_release, std::memory_order_acquire));

        return true;
    }
}
