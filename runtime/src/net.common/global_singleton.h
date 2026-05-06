#pragma once

namespace net::common
{
    template <typename t>
    class global_singleton
    {
    public:

        static t& get_instance()
        {
            static t instance;
            return instance;
        }

        global_singleton(const global_singleton&) = delete;

        global_singleton& operator=(const global_singleton&) = delete;

        global_singleton(global_singleton&&) = delete;

        global_singleton& operator=(global_singleton&&) = delete;

    protected:

        global_singleton() = default;

        virtual ~global_singleton() noexcept = default;
    };
}
