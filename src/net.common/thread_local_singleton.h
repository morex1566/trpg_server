#pragma once

namespace net::common
{
    template <typename t>
    class thread_local_singleton
    {
    public:

        static t& get_instance()
        {
            thread_local t instance;
            return instance;
        }

        thread_local_singleton(const thread_local_singleton&) = delete;

        thread_local_singleton& operator=(const thread_local_singleton&) = delete;

        thread_local_singleton(thread_local_singleton&&) = delete;

        thread_local_singleton& operator=(thread_local_singleton&&) = delete;

    protected:

        thread_local_singleton() = default;

        virtual ~thread_local_singleton() noexcept = default;
    };
}
