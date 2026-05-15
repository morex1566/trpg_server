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

        time();
        ~time() noexcept override;

        // 마지막 tick() 호출 이후 경과 시간
        float delta_time(time_unit unit = time_unit::SECOND) const 
        {
            switch (unit) 
            {
            case time_unit::MILLISECOND: return dt * 1000.0f;
            case time_unit::SECOND: return dt;
            default: return dt;
            }
        }

        // 현재 프레임의 시작 시각을 기록하고 델타 타임을 업데이트
        void tick();

    private:

        std::chrono::steady_clock::time_point last_tick;
        float dt = 0.0f;
    };
}
