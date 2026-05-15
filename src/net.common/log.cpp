#include "pch.h"
#include "log.h"

namespace net::common
{
    log::log() : singleton()
    {
        SPDLOG_INFO("create {} instance.", typeid(net::common::log).name());
    }

    log::~log()
    {

    }
}
