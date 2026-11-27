#include "HeaterFaults.h"

#include <cstdio>

namespace esphome::sunhouse_climate::HeaterFaults
{
namespace
{
const char* Name(const int code)
{
    switch (code)
    {
    case 20:
        return "Thermistor failure";
    case 21:
        return "Core thermistor failure";
    case 22:
        return "Display thermistor failure";
    case 30:
        return "Room too hot";
    case 32:
        return "Display overheat";
    case 34:
        return "Fan failure";
    case 40:
        return "Internal comms failure";
    case 50:
        return "Open window detected";
    case 52:
        return "Heating disabled";
    case 53:
        return "Clock lost (power cut)";
    case 60:
        return "Wireless comms failure";
    case 62:
        return "Charge control signal failure";
    case 63:
        return "Charge times failure";
    default:
        return "Unknown fault";
    }
}
}

std::string Describe(const int code)
{
    if (code == 0)
    {
        return "None";
    }

    // snprintf rather than std::format, which would add about 340 KB to the firmware
    char text[48];
    std::snprintf(text, sizeof(text), "%s (%d)", Name(code), code);
    return text;
}
}
