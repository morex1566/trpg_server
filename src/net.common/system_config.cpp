#include "system_config.h"

#include <sys/sysinfo.h>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

namespace net::common
{
    uint64_t system_config::available_ram_bytes()
    {
        struct sysinfo memInfo;
        if (sysinfo(&memInfo) != 0) return 0;
        return static_cast<uint64_t>(memInfo.freeram) * memInfo.mem_unit;
    }

    double system_config::available_ram_percent()
    {
        struct sysinfo memInfo;
        if (sysinfo(&memInfo) != 0) return 0.0;
        double total = static_cast<double>(memInfo.totalram) * memInfo.mem_unit;
        double free = static_cast<double>(memInfo.freeram) * memInfo.mem_unit;
        return total > 0.0 ? (100.0 * free / total) : 0.0;
    }

    double system_config::cpu_usage_percent()
    {
        auto get_cpu_times = []() -> std::pair<uint64_t, uint64_t> 
        {
            std::ifstream proc_stat("/proc/stat");
            std::string line;
            std::getline(proc_stat, line);

            if (line.compare(0, 3, "cpu") == 0) {
                unsigned long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0, guest_nice = 0;
                if (sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                    &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice) >= 4) {

                    uint64_t idle_time = idle + iowait;
                    uint64_t non_idle_time = user + nice + system + irq + softirq + steal;
                    return { idle_time, idle_time + non_idle_time };
                }
            }

            return { 0, 0 };
        };

        auto [idle1, total1] = get_cpu_times();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto [idle2, total2] = get_cpu_times();

        uint64_t total_diff = total2 - total1;
        uint64_t idle_diff = idle2 - idle1;

        if (total_diff == 0) return 0.0;
        return 100.0 * static_cast<double>(total_diff - idle_diff) / static_cast<double>(total_diff);
    }

}