#pragma once
#include <cstddef>
#include <cstdint>

namespace net::common
{
    /// <summary>
    /// 서버 전역 설정값 제공
    /// </summary>
    class system_config
    {
    public:

        /// <summary>
        /// 서버가 허용하는 최대 RAM 사용률
        /// </summary>
        static constexpr double limit_ram_percentage = 92.5f;


        /// <summary>
        /// tcp 설정값
        /// </summary>
        class tcp
        {
        private:

            /// <summary>
            /// tcp tick rate
            /// </summary>
            static constexpr float tick_rate = 15.f;

        public:

            /// <summary>
            /// tcp tick interval(ms)
            /// </summary>
            static constexpr float tick_interval_ms = 1000.f / tick_rate;

        };

        /// <summary>
        /// connection 설정값
        /// </summary>
        class connection
        {
        public:

            /// <summary>
            /// connection 단위 최대 buffer size
            /// </summary>
            static constexpr size_t buffer_size = 64 * 1024;

            /// <summary>
            /// connection 단위 최대 queue size
            /// </summary>
            static constexpr size_t queue_size = 1024;

            /// <summary>
            /// connection buffer alignment
            /// </summary>
            static constexpr size_t buffer_alignment = 16;
        };

        /// <summary>
        /// tcp accept rate limit 설정값
        /// </summary>
        class tcp_accept_token_bucket
        {
        public:

            /// <summary>
            /// accept token bucket capacity
            /// </summary>
            static constexpr uint64_t capacity = 100;

            /// <summary>
            /// accept token refill interval(ms)
            /// </summary>
            static constexpr double refill_interval_ms = 10.0;
        };

    public:

        /// <summary>
        /// 현재 RAM 사용률 반환
        /// </summary>
        static double current_ram_percentage();

    };
}
