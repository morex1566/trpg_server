#include "system_config.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/sysinfo.h>
#endif

namespace net::common
{
    double system_config::current_ram_percentage()
    {
#ifdef _WIN32
        MEMORYSTATUSEX mem_info{};
        mem_info.dwLength = sizeof(mem_info);

        if (!GlobalMemoryStatusEx(&mem_info))
        {
            return 0.0;
        }

        const double total = static_cast<double>(mem_info.ullTotalPhys);
        const double available = static_cast<double>(mem_info.ullAvailPhys);

        return total > 0.0 ? (100.0 * (total - available) / total) : 0.0;

#else
        struct sysinfo mem_info {};

        if (sysinfo(&mem_info) != 0)
        {
            return 0.0;
        }

        const double total = static_cast<double>(mem_info.totalram) * static_cast<double>(mem_info.mem_unit);
        const double available = static_cast<double>(mem_info.freeram + mem_info.bufferram) * static_cast<double>(mem_info.mem_unit);

        return total > 0.0 ? (100.0 * (total - available) / total) : 0.0;
#endif
    }
}
