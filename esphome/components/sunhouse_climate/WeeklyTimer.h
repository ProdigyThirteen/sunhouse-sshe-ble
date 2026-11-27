#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The heater's weekly timers (register S0/1002, see docs/registers.md), one day at a time.
// In Home Assistant a day is written as up to four periods, such as "7-8:30, 17-22", or "off".
namespace esphome::sunhouse_climate::WeeklyTimer
{
constexpr int kRecordSize = 31;
constexpr int kOutAllDay = 1;
constexpr int kUserTimer = 3;

// The request that makes the next read of the register return this day.
std::vector<std::uint8_t> ReadRequest(int timerMode, int day);

// A day record as text, such as "8-10, 11:30-13". "off" for a day with no periods, and an empty
// string if the record is too short to be one.
std::string FormatDay(const std::vector<std::uint8_t>& record);

// The record that writes text to one day. previous is the day as last read, so each period keeps
// the temperature the heater stored with it; it may be empty. Fails, saying why, on text that
// isn't up to four ordered periods within the day.
[[nodiscard]] std::optional<std::vector<std::uint8_t>> BuildWrite(const std::string& text, int timerMode, int day,
                                                                  const std::vector<std::uint8_t>& previous,
                                                                  std::string* outError = nullptr);
}
