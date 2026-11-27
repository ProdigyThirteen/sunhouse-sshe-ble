#include "SunhouseClimate.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::sunhouse_climate
{
namespace
{
constexpr const char* kTag = "sunhouse_climate";

// ESPHome's LOG_ macros look for a constant with this exact name
constexpr const char* TAG = kTag;

constexpr const char* kPresetOutAllDay = "Out All Day";
constexpr const char* kPresetUserTimer = "User Timer";
constexpr const char* kPresetHoliday = "Holiday";
}

void SunhouseClimate::setup()
{
    set_supported_custom_presets({kPresetOutAllDay, kPresetUserTimer, kPresetHoliday});
    mode = climate::CLIMATE_MODE_HEAT;

    if (_roomSensor != nullptr)
    {
        _roomSensor->add_on_state_callback([this](const float value) {
            current_temperature = value;
            publish_state();
        });

        if (_roomSensor->has_state())
        {
            current_temperature = _roomSensor->state;
        }
    }

    _targetNumber->add_on_state_callback([this](const float value) {
        target_temperature = value;
        publish_state();
    });

    _modeSelect->add_on_state_callback([this](size_t) {
        SyncFromModeSelect();
        publish_state();
    });

    if (_targetNumber->has_state())
    {
        target_temperature = _targetNumber->state;
    }

    SyncFromModeSelect();
    publish_state();
}

void SunhouseClimate::dump_config()
{
    LOG_CLIMATE("", "Sunhouse SSHE thermostat", this);
    ESP_LOGCONFIG(kTag, "  Room sensor: %s", _roomSensor != nullptr ? "yes" : "no");
}

climate::ClimateTraits SunhouseClimate::traits()
{
    auto traits = climate::ClimateTraits();

    if (_roomSensor != nullptr)
    {
        traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
    }

    // Heat only. Holiday is a preset and a switch of its own, so there's no need for "off" to mean Holiday.
    traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
    traits.set_visual_min_temperature(5);
    traits.set_visual_max_temperature(30);
    traits.set_visual_target_temperature_step(1.0f);
    traits.set_visual_current_temperature_step(0.1f);
    return traits;
}

void SunhouseClimate::control(const climate::ClimateCall& call)
{
    // Nothing is published here. The YAML entities read the heater back after each write, and
    // their callbacks update this entity.
    if (call.get_target_temperature().has_value())
    {
        auto targetCall = _targetNumber->make_call();
        targetCall.set_value(std::round(*call.get_target_temperature()));
        targetCall.perform();
    }

    if (call.has_custom_preset())
    {
        const StringRef preset = call.get_custom_preset();

        for (const char* known : {kPresetOutAllDay, kPresetUserTimer, kPresetHoliday})
        {
            if (preset == known)
            {
                SelectPreset(known);
            }
        }
    }
}

void SunhouseClimate::SelectPreset(const char* const preset)
{
    auto selectCall = _modeSelect->make_call();
    selectCall.set_option(preset);
    selectCall.perform();
}

void SunhouseClimate::SyncFromModeSelect()
{
    if (!_modeSelect->has_state())
    {
        return;
    }

    const StringRef option = _modeSelect->current_option();

    for (const char* known : {kPresetOutAllDay, kPresetUserTimer, kPresetHoliday})
    {
        if (option == known)
        {
            set_custom_preset_(known);
        }
    }
}
}
