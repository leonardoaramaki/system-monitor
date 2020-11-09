#include "processor.h"
#include "linux_parser.h"

Processor::Processor(std::string cpuId): cpuId_(cpuId) {}

float Processor::Utilization() {
    time_t now = time(0);
    if (now > last_updated + 2) {
        long active = LinuxParser::ActiveJiffies();
        long idle = LinuxParser::IdleJiffies();
        long prevTotal = active_ + idle_;
        long total = active + idle;
        long totald = total - prevTotal;
        long idled = idle - idle_;
        usage_ = (totald - idled) / float(totald);
        idle_ = idle;
        active_ = active;
    }
    return usage_; 
}