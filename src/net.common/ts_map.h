#pragma once
#include <shared_mutex>
#include <mutex>
#include <memory>
#include <unordered_map>

namespace net::common
{
    // 다중 읽기 가능, thread-safe 쓰기 가능한 해쉬맵
    template <typename key_t, typename value_t>
    class ts_map
    {
    public:
        ts_map() = default;
        ~ts_map() = default;



        void insert(const key_t& key, const value_t& value)
        {
            std::unique_lock<std::shared_mutex> lock(map_mutex);
            map_data[key] = value;
        }

        bool get(const key_t& key, value_t& out_value) const
        {
            std::shared_lock<std::shared_mutex> lock(map_mutex);

            auto it = map_data.find(key);
            if (it != map_data.end())
            {
                out_value = it->second;
                return true;
            }

            return false;
        }

        // Convenience method
        value_t find(const key_t& key) const
        {
            std::shared_lock<std::shared_mutex> lock(map_mutex);

            auto it = map_data.find(key);
            if (it != map_data.end())
            {
                return it->second;
            }

            return value_t();
        }

        void erase(const key_t& key)
        {
            std::scoped_lock<std::shared_mutex> lock(map_mutex);

            map_data.erase(key);
        }

        bool contains(const key_t& key) const
        {
            std::shared_lock<std::shared_mutex> lock(map_mutex);

            return map_data.find(key) != map_data.end();
        }


        std::unordered_map<key_t, value_t> map_data;

        // 읽기 모드 lock X 구현.
        mutable std::shared_mutex map_mutex;
    };
}