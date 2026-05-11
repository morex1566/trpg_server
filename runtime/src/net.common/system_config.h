#pragma once
#include <cstdint>
#include <cstddef>

namespace net::common
{
    class system_config
    {
    public:

        static constexpr double limit_ram_percentage = 92.5f;


        class tcp
        {
        private:
            static constexpr float tick_rate = 15.f;

        public:
            static constexpr float tick_interval_ms = 1000.f / tick_rate;

        };

        class connection
        {
        public:
            static constexpr size_t buffer_size = 64 * 1024;
            static constexpr size_t queue_size = 1024;
            static constexpr size_t buffer_alignment = 16;
        };

        class tcp_accept_token_bucket
        {
        public:
            static constexpr uint64_t capacity = 100;
            static constexpr double refill_interval_ms = 10.0;
        };

    public:
        static double current_ram_percentage();

    };
}
