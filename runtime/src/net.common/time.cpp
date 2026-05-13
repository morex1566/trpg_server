#include "time.h"

#include "net.common/log.h"

#include <spdlog/spdlog.h>

#include <typeinfo>

namespace net::common
{
    time::time() :
        global_singleton(),
        start_timestamp(std::chrono::steady_clock::now()),
        last_timestamp(start_timestamp),
        delta_time(std::chrono::steady_clock::duration::zero())
    {
        SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::common::time).name()));
    }

    time::~time() noexcept
    {
    }

    float time::deltatime(time_unit_type unit) const
    {
        switch (unit)
        {
        case time_unit_type::millisecond:
            return std::chrono::duration<float, std::milli>(delta_time).count();

        case time_unit_type::second:
        default:
            return std::chrono::duration<float>(delta_time).count();
        }
    }

    uint64_t time::timestamp(time_unit_type unit) const
    {
        const auto now = std::chrono::steady_clock::now();
        const auto duration = now - start_timestamp;

        switch (unit)
        {
        case time_unit_type::second:
            return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());

        case time_unit_type::millisecond:
        default:
            return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        }
    }

    void time::update()
    {
        const auto now = std::chrono::steady_clock::now();

        delta_time = now - last_timestamp;
        last_timestamp = now;
    }
}
