#pragma once

#include "esphome/core/component.h"
#include "esphome/components/tuya_low_power/tuya_low_power.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome::tuya_low_power {

class TuyaLPSensor final : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_sensor_id(uint8_t sensor_id) { this->sensor_id_ = sensor_id; }

  void set_tuya_parent(TuyaLowPower *parent) { this->parent_ = parent; }

 protected:
  TuyaLowPower *parent_;
  uint8_t sensor_id_{0};
};

}  // namespace esphome::tuya_low_power
