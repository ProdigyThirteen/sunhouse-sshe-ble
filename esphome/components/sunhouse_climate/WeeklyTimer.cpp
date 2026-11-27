#include "WeeklyTimer.h"

#include <cstddef>
#include <cstdio>
#include <string_view>

namespace esphome::sunhouse_climate::WeeklyTimer
{
namespace
{
constexpr int kPeriodCount = 4;
constexpr int kFirstPeriod = 3;
constexpr int kPeriodSize = 6;
constexpr std::uint8_t kSpareTemperature = 21;
constexpr std::uint8_t kWriteFlag = 0x01;

struct Period
{
    int StartMinute = 0;
    int EndMinute = 0;
};

bool Fail(std::string* outError, std::string message)
{
    if (outError != nullptr)
    {
        *outError = std::move(message);
    }

    return false;
}

// snprintf rather than std::format throughout: std::format adds about 340 KB to the firmware, a fifth
// of the ESP32's app partition.
std::string FormatTime(const int hour, const int minute)
{
    char text[8];
    std::snprintf(text, sizeof(text), minute == 0 ? "%d" : "%d:%02d", hour, minute);
    return text;
}

std::string Quote(const std::string_view item)
{
    return "\"" + std::string(item) + "\"";
}

std::string_view Trim(std::string_view text)
{
    while (!text.empty() && text.front() == ' ')
    {
        text.remove_prefix(1);
    }

    while (!text.empty() && text.back() == ' ')
    {
        text.remove_suffix(1);
    }

    return text;
}

// "8", "08", "8:30" or "08:30" as minutes since midnight. "24" is accepted as the end of the day.
std::optional<int> ParseTime(const std::string_view text)
{
    const std::size_t colon = text.find(':');
    const std::string_view hourText = text.substr(0, colon);
    const std::string_view minuteText = colon == std::string_view::npos ? std::string_view("0") : text.substr(colon + 1);

    if (hourText.empty() || hourText.size() > 2 || minuteText.empty() || minuteText.size() > 2)
    {
        return std::nullopt;
    }

    int hour = 0;
    int minute = 0;

    for (const char digit : hourText)
    {
        if (digit < '0' || digit > '9')
        {
            return std::nullopt;
        }

        hour = hour * 10 + (digit - '0');
    }

    for (const char digit : minuteText)
    {
        if (digit < '0' || digit > '9')
        {
            return std::nullopt;
        }

        minute = minute * 10 + (digit - '0');
    }

    if (minute > 59 || hour > 24 || (hour == 24 && minute != 0))
    {
        return std::nullopt;
    }

    // The heater has no 24:00, so midnight at the end of a day is its last minute.
    return hour == 24 ? 23 * 60 + 59 : hour * 60 + minute;
}

bool ParseDay(const std::string_view text, std::vector<Period>& outPeriods, std::string* outError)
{
    const std::string_view trimmed = Trim(text);

    if (trimmed.empty() || trimmed == "off" || trimmed == "Off" || trimmed == "OFF")
    {
        return true;
    }

    std::size_t start = 0;

    while (start <= trimmed.size())
    {
        const std::size_t comma = trimmed.find_first_of(",;", start);
        const std::string_view item = Trim(trimmed.substr(start, comma == std::string_view::npos ? comma : comma - start));

        if (!item.empty())
        {
            const std::size_t dash = item.find('-');
            const std::optional<int> from = dash == std::string_view::npos ? std::nullopt : ParseTime(Trim(item.substr(0, dash)));
            const std::optional<int> to = dash == std::string_view::npos ? std::nullopt : ParseTime(Trim(item.substr(dash + 1)));

            if (!from || !to)
            {
                return Fail(outError, "can't read " + Quote(item) + ": write periods like 8-10 or 8:30-10:15");
            }

            if (*from >= *to)
            {
                return Fail(outError, Quote(item) + " ends before it starts; periods can't run past midnight");
            }

            outPeriods.push_back({*from, *to});
        }

        if (comma == std::string_view::npos)
        {
            break;
        }

        start = comma + 1;
    }

    if (outPeriods.size() > kPeriodCount)
    {
        return Fail(outError, "a day has room for four periods at most");
    }

    for (std::size_t i = 1; i < outPeriods.size(); ++i)
    {
        if (outPeriods[i].StartMinute < outPeriods[i - 1].EndMinute)
        {
            return Fail(outError, "periods have to be in order without overlapping");
        }
    }

    return true;
}
}

std::vector<std::uint8_t> ReadRequest(const int timerMode, const int day)
{
    return {0x00, static_cast<std::uint8_t>(timerMode), static_cast<std::uint8_t>(day)};
}

std::string FormatDay(const std::vector<std::uint8_t>& record)
{
    if (record.size() < static_cast<std::size_t>(kFirstPeriod + kPeriodCount * kPeriodSize))
    {
        return "";
    }

    std::string text;

    for (int period = 0; period < kPeriodCount; ++period)
    {
        const std::size_t at = static_cast<std::size_t>(kFirstPeriod + period * kPeriodSize);
        const int startHour = record[at];
        const int startMinute = record[at + 1];
        const int endHour = record[at + 2];
        const int endMinute = record[at + 3];

        // Spare periods start and end at the same time
        if (startHour == endHour && startMinute == endMinute)
        {
            continue;
        }

        if (!text.empty())
        {
            text += ", ";
        }

        text += FormatTime(startHour, startMinute) + "-" + FormatTime(endHour, endMinute);
    }

    return text.empty() ? "off" : text;
}

std::optional<std::vector<std::uint8_t>> BuildWrite(const std::string& text, const int timerMode, const int day,
                                                    const std::vector<std::uint8_t>& previous, std::string* outError)
{
    std::vector<Period> periods;

    if (!ParseDay(text, periods, outError))
    {
        return std::nullopt;
    }

    std::vector<std::uint8_t> record(kRecordSize, 0);
    record[0] = kWriteFlag;
    record[1] = static_cast<std::uint8_t>(timerMode);
    record[2] = static_cast<std::uint8_t>(day);

    for (int period = 0; period < kPeriodCount; ++period)
    {
        const std::size_t at = static_cast<std::size_t>(kFirstPeriod + period * kPeriodSize);
        const std::size_t count = periods.size();
        const std::size_t index = static_cast<std::size_t>(period);
        std::uint8_t temperature = kSpareTemperature;

        if (index < count)
        {
            record[at] = static_cast<std::uint8_t>(periods[index].StartMinute / 60);
            record[at + 1] = static_cast<std::uint8_t>(periods[index].StartMinute % 60);
            record[at + 2] = static_cast<std::uint8_t>(periods[index].EndMinute / 60);
            record[at + 3] = static_cast<std::uint8_t>(periods[index].EndMinute % 60);

            if (previous.size() > at + 4 && previous[at + 4] != 0)
            {
                temperature = previous[at + 4];
            }
        }
        else if (count > 0)
        {
            // The heater expects spare periods to sit at the end of the last real one
            const int lastEnd = periods.back().EndMinute;
            record[at] = record[at + 2] = static_cast<std::uint8_t>(lastEnd / 60);
            record[at + 1] = record[at + 3] = static_cast<std::uint8_t>(lastEnd % 60);
        }

        record[at + 4] = temperature;
    }

    return record;
}
}
