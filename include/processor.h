#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <string>
#include <ctime>

class Processor {
 public:
    Processor(std::string cpuId);
 public:
  float Utilization();

 private:
    // This is the cpu identification. Usually named as cpu0, cpu1, cpu2, cpu3, and so on.
    // If identification is "cpu" without any trailing number then it refers to the aggregated cpus.
    std::string cpuId_;
    long idle_{0};
    long active_{0};
    float usage_{0.0};
    time_t last_updated{0};
};

#endif