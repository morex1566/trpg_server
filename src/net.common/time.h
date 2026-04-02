#pragma once
#include "net.common/singleton.h"
#include <chrono>

namespace net::common
{
    /// <summary>
    /// 전역 시간 관리 및 델타 타임 계산 클래스
    /// </summary>
    class time : public singleton<time>
    {
    public:

        enum class time_unit 
        {
            SECOND,
            MILLISECOND
        };

    public:

        time();
        ~time() noexcept override;

        float delta_time(time_unit unit = time_unit::SECOND) const 
        {
            switch (unit) 
            {
            case time_unit::MILLISECOND: return dt * 1000.0f;
            case time_unit::SECOND: return dt;
            default: return dt;
            }
        }

        void tick();

    private:

        std::chrono::steady_clock::time_point last_tick;
        float dt = 0.0f;
    };
}
