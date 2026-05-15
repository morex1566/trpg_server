#include "pch.h"
#include "system_config.h"

#include <windows.h>
#include <pdh.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "pdh.lib")

namespace net::common
{
    uint64_t system_config::available_ram_bytes()
    {
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return static_cast<uint64_t>(status.ullAvailPhys);
    }

    double system_config::available_ram_percent()
    {
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        // 전체 대비 사용 가능한 비율
        return 100.0 * static_cast<double>(status.ullAvailPhys) / static_cast<double>(status.ullTotalPhys);
    }

    double system_config::cpu_usage_percent()
    {
        // PDH 쿼리로 전체 CPU 사용률 샘플링 (100ms 간격)
        PDH_HQUERY query;
        PDH_HCOUNTER counter;

        PdhOpenQuery(nullptr, 0, &query);
        PdhAddEnglishCounter(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter);
        PdhCollectQueryData(query);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        PdhCollectQueryData(query);

        PDH_FMT_COUNTERVALUE value;
        PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value);
        PdhCloseQuery(query);

        return value.doubleValue;
    }

}