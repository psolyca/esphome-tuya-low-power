#pragma once

#include "esphome/core/component.h"
#include "esphome/components/tuya_low_power/tuya_low_power.h"
#include "esphome/components/switch/switch.h"

namespace esphome::tuya_low_power {

class TuyaLPSwitch final : public switch_::Switch, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_switch_id(uint8_t switch_id) { this->switch_id_ = switch_id; }

  void set_tuya_parent(TuyaLowPower *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;

  TuyaLowPower *parent_;
  uint8_t switch_id_{0};
};

}  // namespace esphome::tuya_low_power
