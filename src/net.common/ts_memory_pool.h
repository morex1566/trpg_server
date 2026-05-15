#pragma once
#include "net.common/system_config.h"
#include "net.common/singleton.h"
#include <boost/lockfree/stack.hpp>

namespace net::common
{
    class ts_memory_pool : public singleton<ts_memory_pool>
    {
    public:

        ts_memory_pool();
        ~ts_memory_pool();

        std::shared_ptr<uint8_t> rent(size_t size);

    private:

        // 필요한 크기에 따른 풀 선택 및 할당 헬퍼
        template<typename block_type>
        std::shared_ptr<uint8_t> rent_block(boost::lockfree::stack<block_type*, boost::lockfree::fixed_sized<false>>& stack)
        {
            block_type* block = nullptr;
            if (!stack.pop(block) && can_allocate_dynamic())
            {
                block = new block_type();
            }

            if (block)
            {
                return std::shared_ptr<uint8_t>(reinterpret_cast<uint8_t*>(block),
                    [this, &stack](uint8_t* p)
                    {
                        if (!this->push(reinterpret_cast<block_type*>(p), stack))
                        {
                            delete reinterpret_cast<block_type*>(p);
                        }
                    });
            }
            return nullptr;
        }

        template<typename block_type>
        block_type* pop(boost::lockfree::stack<block_type*, boost::lockfree::fixed_sized<false>>& stack)
        {
            block_type* ptr = nullptr;
            if (stack.pop(ptr)) return ptr;

            return nullptr;
        }

        template<typename block_type>
        bool push(block_type* ptr, boost::lockfree::stack<block_type*, boost::lockfree::fixed_sized<false>>& stack)
        {
            return stack.push(ptr);
        }

        template<typename block_type>
        void initialize_pool(boost::lockfree::stack<block_type*, boost::lockfree::fixed_sized<false>>& stack, size_t count);

        // system_config RAM 제한에 따른 동적 할당 가능 여부 체크
        bool can_allocate_dynamic();

    private:

        boost::lockfree::stack<block64*, boost::lockfree::fixed_sized<false>> pool_64b
        { system_config::ts_memory_pool::pool_64b_count };

        boost::lockfree::stack<block128*, boost::lockfree::fixed_sized<false>> pool_128b
        { system_config::ts_memory_pool::pool_128b_count };

        boost::lockfree::stack<block256*, boost::lockfree::fixed_sized<false>> pool_256b
        { system_config::ts_memory_pool::pool_256b_count };

        boost::lockfree::stack<block512*, boost::lockfree::fixed_sized<false>> pool_512b
        { system_config::ts_memory_pool::pool_512b_count };

        boost::lockfree::stack<block1024*, boost::lockfree::fixed_sized<false>> pool_1024b
        { system_config::ts_memory_pool::pool_1024b_count };

        boost::lockfree::stack<block2048*, boost::lockfree::fixed_sized<false>> pool_2048b
        { system_config::ts_memory_pool::pool_2048b_count };

        boost::lockfree::stack<block4096*, boost::lockfree::fixed_sized<false>> pool_4096b
        { system_config::ts_memory_pool::pool_4096b_count };
    };
}
