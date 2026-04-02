#pragma once
#include "net.common/unit.h"
#include <cstdint>

namespace net::common
{
    // 시스템 하드웨어 상태 조회 유틸리티
    class system_config
    {
    public:

        class tcp
        {
        public:

            static constexpr uint8_t tick_rate = 30;
        };

        class session
        {
        public:

            static constexpr uint16_t packets_per_user = 128;
        };

        class connection
        {
        public:

            static constexpr double memory_alloc_limit_percent = 80.f;
            static constexpr size_t expected_ccu = 1000;
            static constexpr size_t buffer_size_per_user = 64 * 1024;
            static constexpr size_t queue_size_per_user = 1024;
            static constexpr size_t buffer_alignment = 16;
            static constexpr size_t default_buffer_size = expected_ccu * buffer_size_per_user;
        };

        class ts_memory_pool
        {
        public:

            static constexpr size_t pool_64b_count = (8 * KB) / 64;
            static constexpr size_t pool_128b_count = (16 * KB) / 128;
            static constexpr size_t pool_256b_count = (32 * KB) / 256;
            static constexpr size_t pool_512b_count = (32 * KB) / 512;
            static constexpr size_t pool_1024b_count = (16 * KB) / 1024;
            static constexpr size_t pool_2048b_count = (8 * KB) / 2048;
            static constexpr size_t pool_4096b_count = (8 * KB) / 4096;
        };

    public:

        static uint64_t available_ram_bytes();
        static double available_ram_percent();
        static double cpu_usage_percent();
    };
}