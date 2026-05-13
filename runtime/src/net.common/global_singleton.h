#pragma once

namespace net::common
{
    /// <summary>
    /// 프로세스 전역에서 하나의 인스턴스를 제공하는 싱글톤 베이스
    /// </summary>
    template <typename t>
    class global_singleton
    {
    public:

        /// <summary>
        /// 전역 싱글톤 인스턴스 반환
        /// </summary>
        static t& get_instance()
        {
            static t instance;
            return instance;
        }

        /// <summary>
        /// 싱글톤 복사 생성 방지
        /// </summary>
        global_singleton(const global_singleton&) = delete;

        /// <summary>
        /// 싱글톤 복사 대입 방지
        /// </summary>
        global_singleton& operator=(const global_singleton&) = delete;

        /// <summary>
        /// 싱글톤 이동 생성 방지
        /// </summary>
        global_singleton(global_singleton&&) = delete;

        /// <summary>
        /// 싱글톤 이동 대입 방지
        /// </summary>
        global_singleton& operator=(global_singleton&&) = delete;

    protected:

        /// <summary>
        /// 파생 클래스에서만 생성 가능
        /// </summary>
        global_singleton() = default;

        /// <summary>
        /// 파생 클래스 소멸 시 가상 소멸 지원
        /// </summary>
        virtual ~global_singleton() noexcept = default;
    };
}
