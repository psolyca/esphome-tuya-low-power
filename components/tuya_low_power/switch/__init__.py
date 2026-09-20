import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_SWITCH_DATAPOINT

from .. import CONF_TUYA_LOW_POWER_ID, TuyaLowPower , tuya_low_power_ns

DEPENDENCIES = ["tuya_low_power"]
CODEOWNERS = ["@jesserockz"]

TuyaLPSwitch = tuya_low_power_ns.class_("TuyaLPSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = (
    switch.switch_schema(TuyaLPSwitch)
    .extend(
        {
            cv.GenerateID(CONF_TUYA_LOW_POWER_ID): cv.use_id(TuyaLowPower ),
            cv.Required(CONF_SWITCH_DATAPOINT): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_TUYA_LOW_POWER_ID])
    cg.add(var.set_tuya_parent(paren))

    cg.add(var.set_switch_id(config[CONF_SWITCH_DATAPOINT]))
