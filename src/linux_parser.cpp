#include <dirent.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() { 
  string line, key, value;
  unsigned long memTotal = 0, memFree = 0;
  std::ifstream filestream(kProcDirectory + kMeminfoFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::istringstream stream(line);
      stream >> key >> value;
      if (key == "MemTotal:") {
        memTotal = std::stoul(value);
      } else if (key == "MemFree:") {
        memFree = std::stoul(value);
      }
    }
  }
  return float(memFree) / memTotal; 
}

long LinuxParser::UpTime() { 
  float uptime = 0;
  string line, system, idle;
  std::ifstream filestream(kProcDirectory + kUptimeFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line);
    std::istringstream ss(line);
    ss >> system >> idle;
    float sys_time = stof(system);
    float idle_time = stof(idle);
    uptime = sys_time + idle_time;
  }
  return long(uptime); 
}

long LinuxParser::Jiffies() {
  return ActiveJiffies() + IdleJiffies();
}

// Based on solution provided at: https://stackoverflow.com/questions/16726779/how-do-i-get-the-total-cpu-usage-of-an-application-from-proc-pid-stat/16736599#16736599
long LinuxParser::ActiveJiffies(int pid) { 
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kStatFilename);
  float cpu_usage = 0;
  if (filestream.is_open()) {
    string line, field;
    int count = 1;
    const int kUtime = 14, kStime = 15, kCutime = 16, kCstime = 17, kStarttime = 22;
    long total_time = 0, utime, stime, cutime, cstime, starttime;
    while (std::getline(filestream, line)) {
      std::istringstream stream(line);
      while (stream >> field) {
        switch (count) {
        case kUtime:
          utime = std::stol(field);
          break;
        case kStime:
          stime = std::stol(field);
          break;
        case kCutime:
          cutime = std::stol(field);
          break;
        case kCstime:
          cstime = std::stol(field);
          break;
        case kStarttime:
          starttime = std::stol(field);
          break;
        default:
          break;
        }
        ++count;
        if (count > kStarttime) {
          break;
        }
      }
    }
    // UpTime returns in seconds, so convert it to jiffie
    long uptime = UpTime() * sysconf(_SC_CLK_TCK);
    total_time = utime + stime;
    // Add children processes
    total_time += cutime + cstime;
    long elapsed = uptime - starttime;
    cpu_usage = 100 * (float(total_time) / elapsed);
  }
  return cpu_usage; 
}

// Implementation based on: https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
long LinuxParser::ActiveJiffies() { 
  long active_jiffies = 0;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    string line, state, cpuId;
    // The aggregate cpu jiffies is always at the first line.
    std::getline(filestream, line);
    std::istringstream stream(line);
    // Get the cpu id from the stream.
    stream >> cpuId;
    // Get the cpu states and add up.
    int cpu_state = 0;
    while (stream >> state) {
      switch (cpu_state) {
      case CPUStates::kUser_:
      case CPUStates::kNice_:
      case CPUStates::kSystem_:
      case CPUStates::kIRQ_:
      case CPUStates::kSoftIRQ_:
      case CPUStates::kSteal_:
        active_jiffies += std::stol(state);
        break;
      }
      ++cpu_state;
    }
  }
  return active_jiffies; 
}

// Implementation based on: https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
long LinuxParser::IdleJiffies() { 
  long idle_jiffies = 0;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    string line, state, cpuId;
    // The aggregate cpu jiffies is always at the first line.
    std::getline(filestream, line);
    std::istringstream stream(line);
    // Get the cpu id from the stream.
    stream >> cpuId;
    // Get the cpu states and add up.
    int cpu_state = 0;
    while (stream >> state) {
      switch (cpu_state) {
      case CPUStates::kIdle_:
      case CPUStates::kIOwait_:
        idle_jiffies += std::stol(state);
        break;
      }
      ++cpu_state;
    }
  }
  return idle_jiffies;
}

// Based on solution at https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux.
vector<string> LinuxParser::CpuUtilization() { 
  float cpu_usage = float(ActiveJiffies() / sysconf(_SC_CLK_TCK)) / float(Jiffies() / sysconf(_SC_CLK_TCK));
  return { std::to_string(cpu_usage) };
}

int LinuxParser::TotalProcesses() {
  string line, processes, value;
  int num_processes = 0;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::istringstream stream(line);
      stream >> processes;
      if (processes == "processes" && stream >> value) {
        num_processes = std::stoi(value);
      }
    }
  }
  return num_processes; 
}

int LinuxParser::RunningProcesses() { 
  string line, procs_running, value;
  int running_processes = 0;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::istringstream stream(line);
      stream >> procs_running;
      if (procs_running == "procs_running" && stream >> value) {
        running_processes = std::stoi(value);
      }
    }
  }
  return running_processes;
}

string LinuxParser::Command(int pid) {
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kCmdlineFilename);
  string cmd;
  if (filestream.is_open()) {
    std::getline(filestream, cmd);
  }
  return cmd;
}

string LinuxParser::Ram(int pid) {
  float ram = 0;
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kStatusFilename);
  if (filestream.is_open()) {
    string line, key, value;
    while (std::getline(filestream, line)) {
      std::istringstream stream(line);
      stream >> key >> value;
      if (key == "VmSize:") {
        // Convert kB to mB
        ram = std::stof(value) / (float) 1024;
        break;
      }
    }
  }
  std::ostringstream ans;
  ans.precision(2);
  ans << std::fixed << ram;
  return ans.str(); 
}

string LinuxParser::Uid(int pid) { 
  string uid;
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kStatusFilename);
  if (filestream.is_open()) {
    string line, key;
    while (std::getline(filestream, line)) {
      std::istringstream stream(line);
      stream >> key;
      if (key == "Uid:") {
        stream >> uid;
        break;
      }
    }
  }
  return uid; 
}

string LinuxParser::User(int pid) { 
  string user;
  string pid_uid = Uid(pid);
  std::ifstream filestream(kPasswordPath);
  if (filestream.is_open()) {
    string line, password, uid;
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream stream(line);
      stream >> user >> password >> uid;
      if (uid == pid_uid) {
        break;
      }
    }
  }
  return user;  
}

long LinuxParser::UpTime(int pid) {
  string line, value;
  std::ifstream filestream(kProcDirectory + std::to_string(pid) + kStatFilename);
  long starttime_jiffies{0};
  const int starttime_col{22};
  if (filestream.is_open()) {
    int i = 1;
    while (std::getline(filestream, line)) {
      std::istringstream stream(line);
      while (stream >> value) {
        if (i == starttime_col) break;
        ++i;
      }
      if (i == starttime_col) {
        starttime_jiffies = std::stol(value);
        break;
      }
    }
  }
  return UpTime() - (starttime_jiffies / sysconf(_SC_CLK_TCK)); 
}