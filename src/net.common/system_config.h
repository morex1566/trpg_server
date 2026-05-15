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

            // 서버 -> 클라 패킷 전송 주기
            static constexpr uint8_t tick_rate = 30;
        };

        class session
        {
        public:

            // 유저가 할당받는 패킷 풀 사이즈
            static constexpr uint16_t packets_per_user = 128;
        };

        class connection
        {
        public:

            // 이벤트 기간 등 갑작스러운 유저 유입 대비
            static constexpr double memory_alloc_limit_percent = 80.f;

            // 예상 최대 동시 접속자 수
            static constexpr size_t expected_ccu = 1000;

            // 1인당 예상 메모리 제공량 (32KB)
            static constexpr size_t buffer_size_per_user = 64 * 1024;

            static constexpr size_t queue_size_per_user = 1024;
            
            // 메모리 풀 정렬 단위
            static constexpr size_t buffer_alignment = 16;

            // 초기 풀 전체 용량
            static constexpr size_t default_buffer_size = expected_ccu * buffer_size_per_user;
        };

        class ts_memory_pool
        {
        public:

            // 목표 사이즈 / 청크 사이즈 = 개수
            static constexpr size_t pool_64b_count = (8 * KB) / 64;

            static constexpr size_t pool_128b_count = (16 * KB) / 128;

            static constexpr size_t pool_256b_count = (32 * KB) / 256;

            static constexpr size_t pool_512b_count = (32 * KB) / 512;

            static constexpr size_t pool_1024b_count = (16 * KB) / 1024;

            static constexpr size_t pool_2048b_count = (8 * KB) / 2048;

            static constexpr size_t pool_4096b_count = (8 * KB) / 4096;
        };

    protected:

        //

    private:

        //

    public:

        // 현재 사용 가능한 물리 RAM (bytes)
        static uint64_t available_ram_bytes();

        // 현재 사용 가능한 물리 RAM 비율 (0.0 ~ 100.0)
        static double available_ram_percent();

        // 현재 CPU 사용률 (0.0 ~ 100.0), 100ms 샘플링
        static double cpu_usage_percent();

    protected:

        //

    private:

        //
    };
}