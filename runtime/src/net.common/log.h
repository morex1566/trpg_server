#pragma once
#include "net.common/global_singleton.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <iostream>
#include <typeinfo>
#include <string>

namespace net::common
{
    std::string demangle(const char* name);
}

namespace net::common
{
    class log : public global_singleton<log>
    {
    public:

        log();
        ~log() noexcept override;

        void init();
    };
}
