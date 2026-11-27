#pragma once

#include <string>

namespace esphome::sunhouse_climate::HeaterFaults
{
// A fault code from register S0/1011 in words, such as "Core thermistor failure (21)".
// Code 0 is "None".
std::string Describe(int code);
}
