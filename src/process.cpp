#include <unistd.h>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "process.h"
#include "linux_parser.h"

using std::string;
using std::to_string;
using std::vector;

Process::Process(int pid): pid_(pid) {}

int Process::Pid() { return pid_; }

float Process::CpuUtilization() { 
    long active = LinuxParser::ActiveJiffies(Pid());
    long total = LinuxParser::Jiffies();
    return 100 * (float(active) / total); 
}

string Process::Command() { return LinuxParser::Command(Pid()); }

string Process::Ram() { return LinuxParser::Ram(Pid()); }

string Process::User() { return LinuxParser::User(Pid()); }

long int Process::UpTime() { return LinuxParser::UpTime(Pid()); }

bool Process::operator<(Process const& a) const { 
    return const_cast<Process&>(a).CpuUtilization() < const_cast<Process*>(this)->CpuUtilization(); 
}