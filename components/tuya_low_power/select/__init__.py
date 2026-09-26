import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_INITIAL_OPTION,
    CONF_ENUM_DATAPOINT,
    CONF_INT_DATAPOINT,
    CONF_OPTIMISTIC,
    CONF_OPTIONS,
    CONF_RESTORE_VALUE,
)

from .. import CONF_TUYA_LOW_POWER_ID, TuyaLowPower, tuya_low_power_ns

DEPENDENCIES = ["tuya_low_power"]
CODEOWNERS = ["@bearpawmaxim"]

TuyaLPSelect = tuya_low_power_ns.class_("TuyaLPSelect", select.Select, cg.Component)


def ensure_option_map(value):
    cv.check_not_templatable(value)
    option = cv.All(cv.int_range(0, 2**8 - 1))
    mapping = cv.All(cv.string_strict)
    options_map_schema = cv.Schema({option: mapping})
    value = options_map_schema(value)

    all_values = list(value.keys())
    unique_values = set(value.keys())
    if len(all_values) != len(unique_values):
        raise cv.Invalid("Mapping values must be unique.")

    return value

def validate(config):
    errors = []
    if CONF_INITIAL_OPTION in config:
        if config[CONF_INITIAL_OPTION] not in config[CONF_OPTIONS].values():
            errors.append(
                cv.Invalid(
                    f"initial_option '{config[CONF_INITIAL_OPTION]}' is not a valid option [{', '.join(config[CONF_OPTIONS].values())}]",
                    path=[CONF_INITIAL_OPTION],
                )
            )
    else:
        config[CONF_INITIAL_OPTION] = config[CONF_OPTIONS][0]

    if errors:
        raise cv.MultipleInvalid(errors)

    return config

CONFIG_SCHEMA = cv.All(
    select.select_schema(TuyaLPSelect)
    .extend(
        {
            cv.GenerateID(CONF_TUYA_LOW_POWER_ID): cv.use_id(TuyaLowPower),
            cv.Optional(CONF_ENUM_DATAPOINT): cv.uint8_t,
            cv.Optional(CONF_INT_DATAPOINT): cv.uint8_t,
            cv.Required(CONF_OPTIONS): ensure_option_map,
            cv.Optional(CONF_OPTIMISTIC, default=False): cv.boolean,
            cv.Optional(CONF_INITIAL_OPTION): cv.string_strict,
            cv.Optional(CONF_RESTORE_VALUE, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.has_exactly_one_key(CONF_ENUM_DATAPOINT, CONF_INT_DATAPOINT),
    validate,
)


async def to_code(config):
    options_map = config[CONF_OPTIONS]
    var = await select.new_select(config, options=list(options_map.values()))
    await cg.register_component(var, config)
    cg.add(var.set_select_mappings(list(options_map.keys())))
    parent = await cg.get_variable(config[CONF_TUYA_LOW_POWER_ID])
    cg.add(var.set_tuya_parent(parent))
    if (enum_datapoint := config.get(CONF_ENUM_DATAPOINT, None)) is not None:
        cg.add(var.set_select_id(enum_datapoint, False))
    if (int_datapoint := config.get(CONF_INT_DATAPOINT, None)) is not None:
        cg.add(var.set_select_id(int_datapoint, True))
    cg.add(var.set_optimistic(config[CONF_OPTIMISTIC]))
    if (init_string := config.get(CONF_INITIAL_OPTION, None)) is not None:
        index = list(options_map.keys())[list(options_map.values()).index(init_string)]
        cg.add(var.set_initial_index(index))
    cg.add(var.set_restore_index(config[CONF_RESTORE_VALUE]))
