#pragma once

namespace net::common
{
    /// <summary>
    /// 스레드마다 하나의 인스턴스를 제공하는 싱글톤 베이스
    /// </summary>
    template <typename t>
    class thread_local_singleton
    {
    public:

        /// <summary>
        /// 현재 스레드의 싱글톤 인스턴스 반환
        /// </summary>
        static t& get_instance()
        {
            thread_local t instance;
            return instance;
        }

        /// <summary>
        /// 싱글톤 복사 생성 방지
        /// </summary>
        thread_local_singleton(const thread_local_singleton&) = delete;

        /// <summary>
        /// 싱글톤 복사 대입 방지
        /// </summary>
        thread_local_singleton& operator=(const thread_local_singleton&) = delete;

        /// <summary>
        /// 싱글톤 이동 생성 방지
        /// </summary>
        thread_local_singleton(thread_local_singleton&&) = delete;

        /// <summary>
        /// 싱글톤 이동 대입 방지
        /// </summary>
        thread_local_singleton& operator=(thread_local_singleton&&) = delete;

    protected:

        /// <summary>
        /// 파생 클래스에서만 생성 가능
        /// </summary>
        thread_local_singleton() = default;

        /// <summary>
        /// 파생 클래스 소멸 시 가상 소멸 지원
        /// </summary>
        virtual ~thread_local_singleton() noexcept = default;
    };
}
