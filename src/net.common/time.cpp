
#include "time.h"
#include "log.h"

namespace net::common
{
    time::time() : singleton()
    {
        last_tick = std::chrono::steady_clock::now();

        SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::common::time).name()));
    }

    time::~time()
    {

    }

    void time::tick()
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - last_tick;
        dt = elapsed.count();
        last_tick = now;
    }
}
