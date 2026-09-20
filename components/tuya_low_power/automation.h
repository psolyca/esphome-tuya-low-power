#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "tuya_low_power.h"

#include <vector>

namespace esphome::tuya_low_power {

class TuyaDatapointUpdateTrigger final : public Trigger<TuyaDatapoint> {
 public:
  explicit TuyaDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id) {
    parent->register_listener(sensor_id, [this](const TuyaDatapoint &dp) { this->trigger(dp); });
  }
};

class TuyaRawDatapointUpdateTrigger final : public Trigger<std::vector<uint8_t>> {
 public:
  explicit TuyaRawDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id);
};

class TuyaBoolDatapointUpdateTrigger final : public Trigger<bool> {
 public:
  explicit TuyaBoolDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id);
};

class TuyaIntDatapointUpdateTrigger final : public Trigger<int> {
 public:
  explicit TuyaIntDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id);
};

class TuyaUIntDatapointUpdateTrigger final : public Trigger<uint32_t> {
 public:
  explicit TuyaUIntDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id);
};

class TuyaStringDatapointUpdateTrigger final : public Trigger<std::string> {
 public:
  explicit TuyaStringDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id);
};

class TuyaEnumDatapointUpdateTrigger final : public Trigger<uint8_t> {
 public:
  explicit TuyaEnumDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id);
};

class TuyaBitmaskDatapointUpdateTrigger final : public Trigger<uint32_t> {
 public:
  explicit TuyaBitmaskDatapointUpdateTrigger(TuyaLowPower *parent, uint8_t sensor_id);
};

}  // namespace esphome::tuya_low_power
