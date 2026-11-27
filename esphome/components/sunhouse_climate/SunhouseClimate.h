#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome::sunhouse_climate
{
// One Home Assistant thermostat made from the YAML's entities, which do all the Bluetooth work:
// the room sensor gives the current temperature, the target number the setpoint, and the mode
// select the preset. It only publishes what those entities read back, so it always shows what
// the heater is doing rather than what was asked for.
class SunhouseClimate final : public climate::Climate, public Component
{
public:
    void SetRoomSensor(sensor::Sensor* roomSensor) { _roomSensor = roomSensor; }
    void SetTargetNumber(number::Number* targetNumber) { _targetNumber = targetNumber; }
    void SetModeSelect(select::Select* modeSelect) { _modeSelect = modeSelect; }

    void setup() override;
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::LATE; }

protected:
    climate::ClimateTraits traits() override;
    void control(const climate::ClimateCall& call) override;

private:
    void SelectPreset(const char* preset);
    void SyncFromModeSelect();

    // Optional: without it the thermostat has no current temperature.
    sensor::Sensor* _roomSensor = nullptr;
    number::Number* _targetNumber = nullptr;
    select::Select* _modeSelect = nullptr;
};
}
