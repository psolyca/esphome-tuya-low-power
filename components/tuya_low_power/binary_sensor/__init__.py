import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_SENSOR_DATAPOINT

from .. import CONF_TUYA_LOW_POWER_ID, TuyaLowPower, tuya_low_power_ns

DEPENDENCIES = ["tuya_low_power"]
CODEOWNERS = ["@jesserockz"]

TuyaLPBinarySensor = tuya_low_power_ns.class_(
    "TuyaLPBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(TuyaLPBinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_TUYA_LOW_POWER_ID): cv.use_id(TuyaLowPower),
            cv.Required(CONF_SENSOR_DATAPOINT): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_TUYA_LOW_POWER_ID])
    cg.add(var.set_tuya_parent(paren))

    cg.add(var.set_sensor_id(config[CONF_SENSOR_DATAPOINT]))
