#include "log.h"

#ifndef _WIN32
#include <cxxabi.h>
#include <memory>
#include <cstdlib>
#endif

namespace net::common
{
    std::string demangle(const char* name)
    {
#ifdef _WIN32
        return name;
#else
        int status = 0;
        std::unique_ptr<char, void(*)(void*)> res
        {
            abi::__cxa_demangle(name, nullptr, nullptr, &status),
            std::free
        };
        return (status == 0) ? res.get() : name;
#endif
    }

    log::log() : global_singleton()
    {
        SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::common::log).name()));
    }

    log::~log() noexcept
    {
    }

    void log::init()
    {
        auto console = spdlog::stdout_color_mt("console");
        auto err_logger = spdlog::stderr_color_mt("stderr");
        (void)err_logger;

        // spdlog::set_default_logger(console);
        // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %s:%# - %v");
    }
}
