#pragma once

namespace net::common
{
    template <typename t>
    class singleton
    {
    public:
        // 인스턴스 반환
        static t& get_instance()
        {
            static t instance;
            return instance;
        }

        singleton(const singleton&) = delete;

        singleton& operator=(const singleton&) = delete;

        singleton(singleton&&) = delete;

        singleton& operator=(singleton&&) = delete;

    protected:
        // 상속 전용 생성자
        singleton() = default;

        virtual ~singleton() noexcept = default;
    };
}