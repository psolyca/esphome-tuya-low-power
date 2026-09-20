#include "esphome/core/log.h"

#include "automation.h"

static const char *const TAG = "tuya_low_power.automation";

namespace esphome::tuya_low_power {

void check_expected_datapoint(const TuyaDatapoint &dp, TuyaDatapointType expected) {
  if (dp.type != expected) {
    ESP_LOGW(TAG, "Tuya sensor %u expected datapoint type %#02hhX but got %#02hhX", dp.id,
             static_cast<uint8_t>(expected), static_cast<uint8_t>(dp.type));
  }
}

TuyaRawDatapointUpdateTrigger::TuyaRawDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
  parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) {
    check_expected_datapoint(dp, TuyaDatapointType::RAW);
    this->trigger(dp.value_raw);
  });
}

TuyaBoolDatapointUpdateTrigger::TuyaBoolDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
  parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) {
    check_expected_datapoint(dp, TuyaDatapointType::BOOLEAN);
    this->trigger(dp.value_bool);
  });
}

TuyaIntDatapointUpdateTrigger::TuyaIntDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
  parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) {
    check_expected_datapoint(dp, TuyaDatapointType::INTEGER);
    this->trigger(dp.value_int);
  });
}

TuyaUIntDatapointUpdateTrigger::TuyaUIntDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
  parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) {
    check_expected_datapoint(dp, TuyaDatapointType::INTEGER);
    this->trigger(dp.value_uint);
  });
}

TuyaStringDatapointUpdateTrigger::TuyaStringDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
  parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) {
    check_expected_datapoint(dp, TuyaDatapointType::STRING);
    this->trigger(dp.value_string);
  });
}

TuyaEnumDatapointUpdateTrigger::TuyaEnumDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
  parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) {
    check_expected_datapoint(dp, TuyaDatapointType::ENUM);
    this->trigger(dp.value_enum);
  });
}

TuyaBitmaskDatapointUpdateTrigger::TuyaBitmaskDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
  parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) {
    check_expected_datapoint(dp, TuyaDatapointType::BITMASK);
    this->trigger(dp.value_bitmask);
  });
}

}  // namespace esphome::tuya_low_power
