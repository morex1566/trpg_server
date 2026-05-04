#pragma once
#include "net.common/global_singleton.h"
#include <chrono>
#include <cstdint>

namespace net::common
{
    class time : public global_singleton<time>
    {
    public:

        enum class time_unit_type
        {
            SECOND,
            MILLISECOND
        };

    public:

        time();
        ~time() noexcept override;

        /// <summary>
        /// current timestamp - last timestamp
        /// </summary>
        float deltatime(time_unit_type unit = time_unit_type::SECOND) const;

        /// <summary>
        /// start_timestamp 기준 현재 시간
        /// </summary>
        uint64_t timestamp(time_unit_type unit = time_unit_type::MILLISECOND) const;

        /// <summary>
        /// last_timestamp를 최신화
        /// </summary>
        void update();

    private:

        /// <summary>
        /// time 클래스가 생성된 시간
        /// </summary>
        std::chrono::steady_clock::time_point start_timestamp;

        /// <summary>
        /// 마지막으로 update()가 호출된 시간
        /// </summary>
        std::chrono::steady_clock::time_point last_timestamp;

        /// <summary>
        /// 마지막 update()에서 계산된 프레임 단위 시간
        /// </summary>
        std::chrono::steady_clock::duration delta_time;
    };
}
