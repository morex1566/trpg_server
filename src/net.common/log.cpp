
#include "log.h"

namespace net::common
{
    log::log() : singleton()
    {
        SPDLOG_INFO("create {} instance.", net::common::demangle(typeid(net::common::log).name()));
    }

    log::~log()
    {

    }
}
