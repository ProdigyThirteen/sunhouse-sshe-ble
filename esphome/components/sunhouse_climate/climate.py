import esphome.codegen as cg
from esphome.components import climate, number, select, sensor
import esphome.config_validation as cv

CONF_ROOM_SENSOR = "room_sensor"
CONF_TARGET_NUMBER = "target_number"
CONF_MODE_SELECT = "mode_select"

sunhouse_ns = cg.esphome_ns.namespace("sunhouse_climate")
SunhouseClimate = sunhouse_ns.class_("SunhouseClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = (
    climate.climate_schema(SunhouseClimate)
    .extend(
        {
            cv.Optional(CONF_ROOM_SENSOR): cv.use_id(sensor.Sensor),
            cv.Required(CONF_TARGET_NUMBER): cv.use_id(number.Number),
            cv.Required(CONF_MODE_SELECT): cv.use_id(select.Select),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    if CONF_ROOM_SENSOR in config:
        cg.add(var.SetRoomSensor(await cg.get_variable(config[CONF_ROOM_SENSOR])))

    cg.add(var.SetTargetNumber(await cg.get_variable(config[CONF_TARGET_NUMBER])))
    cg.add(var.SetModeSelect(await cg.get_variable(config[CONF_MODE_SELECT])))
