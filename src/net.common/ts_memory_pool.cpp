
#include "ts_memory_pool.h"
#include "log.h"

namespace net::common
{
    ts_memory_pool::ts_memory_pool()
    {
        initialize_pool(pool_64b, system_config::ts_memory_pool::pool_64b_count);
        initialize_pool(pool_128b, system_config::ts_memory_pool::pool_128b_count);
        initialize_pool(pool_256b, system_config::ts_memory_pool::pool_256b_count);
        initialize_pool(pool_512b, system_config::ts_memory_pool::pool_512b_count);
        initialize_pool(pool_1024b, system_config::ts_memory_pool::pool_1024b_count);
        initialize_pool(pool_2048b, system_config::ts_memory_pool::pool_2048b_count);
        initialize_pool(pool_4096b, system_config::ts_memory_pool::pool_4096b_count);
    }

    ts_memory_pool::~ts_memory_pool()
    {
        // 풀에 남아 있는 모든 블록들을 안전하게 해제합니다.
        block64* b64; while (pool_64b.pop(b64)) delete b64;
        block128* b128; while (pool_128b.pop(b128)) delete b128;
        block256* b256; while (pool_256b.pop(b256)) delete b256;
        block512* b512; while (pool_512b.pop(b512)) delete b512;
        block1024* b1024; while (pool_1024b.pop(b1024)) delete b1024;
        block2048* b2048; while (pool_2048b.pop(b2048)) delete b2048;
        block4096* b4096; while (pool_4096b.pop(b4096)) delete b4096;
    }

    std::shared_ptr<uint8_t> ts_memory_pool::rent(size_t size)
    {
        if (size <= 64)   return rent_block(pool_64b);
        if (size <= 128)  return rent_block(pool_128b);
        if (size <= 256)  return rent_block(pool_256b);
        if (size <= 512)  return rent_block(pool_512b);
        if (size <= 1024) return rent_block(pool_1024b);
        if (size <= 2048) return rent_block(pool_2048b);
        if (size <= 4096) return rent_block(pool_4096b);

        SPDLOG_ERROR("memory rent failed. size: {}, is out of limit?", size);
        return nullptr;
    }

    template<typename block_type>
    void ts_memory_pool::initialize_pool(boost::lockfree::stack<block_type*, boost::lockfree::fixed_sized<false>>& stack, size_t count)
    {
        for (size_t i = 0; i < count; ++i) 
        {
            block_type* block = new block_type();
            if (!stack.push(block)) 
            {
                delete block;
            }
        }
    }

    bool ts_memory_pool::can_allocate_dynamic()
    {
        // RAM 사용 80% 미만일 때만 추가 할당
        return system_config::available_ram_percent() > (100.0 - system_config::connection::memory_alloc_limit_percent);
    }
}
