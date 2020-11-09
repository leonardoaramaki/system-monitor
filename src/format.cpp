#include <string>
#include <sstream>
#include "format.h"

using std::string;

string Format::ElapsedTime(long seconds) {
    string out;
    int hours, minutes;
    minutes = seconds / 60;
    hours = minutes / 60;
    std::ostringstream stream;
    stream << hours << ":" << minutes % 60 << ":" << seconds % 60;
    return stream.str(); 
}