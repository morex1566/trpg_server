#ifndef PCH_H
#define PCH_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include "net.common/log.h"

#include "flatbuffers/flatbuffers.h"
#include "spdlog/spdlog.h"
#include "boost/asio.hpp"

#endif //PCH_H
