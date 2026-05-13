#pragma once
#include "net.common/global_singleton.h"

#include <string>

#include <spdlog/spdlog.h>

namespace net::common
{
    /// <summary>
    /// C++ 타입 이름을 사람이 읽기 쉬운 이름으로 변환
    /// </summary>
    std::string demangle(const char* name);
}

namespace net::common
{
    /// <summary>
    /// 서버 로그 시스템 초기화 담당
    /// </summary>
    class log : public global_singleton<log>
    {
    public:

        /// <summary>
        /// log 인스턴스 생성
        /// </summary>
        log();

        /// <summary>
        /// log 인스턴스 소멸
        /// </summary>
        ~log() noexcept override;

        /// <summary>
        /// spdlog sink 및 logger 초기화
        /// </summary>
        void init();
    };
}
