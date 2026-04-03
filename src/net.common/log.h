#pragma once
#include "net.common/singleton.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <iostream>
#include <typeinfo>
#include <string>

#ifndef _WIN32
#include <cxxabi.h>
#include <memory>
#include <cstdlib>
#endif

namespace net::common
{
    inline std::string demangle(const char* name)
    {
#ifdef _WIN32
        return name;
#else
        int status = 0;
        std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(name, nullptr, nullptr, &status),
            std::free
        };
        return (status == 0) ? res.get() : name;
#endif
    }
}

#define CHECK_RETURN_VOID_ERROR(condition, ...) \
    do { \
        if (condition) \
		{ \
            SPDLOG_ERROR(__VA_ARGS__); \
			return; \
        } \
    } while (0)

#define CHECK_RETURN_VOID(condition) \
    do { \
        if (condition) \
		{ \
			return; \
        } \
    } while (0)

#define CHECK_RETURN(condition, ret_val, ...) \
    do { \
        if (condition) \
		{ \
            SPDLOG_ERROR(__VA_ARGS__); \
			return ret_val; \
        } \
    } while (0)

#define CHECK_CONTINUE(condition, ...) \
    do { \
        if (condition) \
		{ \
            SPDLOG_ERROR(__VA_ARGS__); \
        } \
    } while (0)

namespace net::common
{
	class log : public singleton<log>
	{
	public:
        log();
        ~log() noexcept override;

		void init()
		{
			auto console = spdlog::stdout_color_mt("console");
			auto err_logger = spdlog::stderr_color_mt("stderr");

			spdlog::set_default_logger(console);
			spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %s:%# - %v");
		}
	};
}