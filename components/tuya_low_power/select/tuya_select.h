#pragma once

#include "esphome/core/component.h"
#include "esphome/components/tuya_low_power/tuya_low_power.h"
#include "esphome/components/select/select.h"

#include <vector>

namespace esphome::tuya_low_power {

class TuyaLPSelect final : public select::Select, public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_tuya_parent(TuyaLowPower *parent) { this->parent_ = parent; }
  void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }
  void set_select_id(uint8_t select_id, bool is_int) {
    this->select_id_ = select_id;
    this->is_int_ = is_int;
  }
  void set_select_mappings(std::vector<uint8_t> mappings) { this->mappings_ = std::move(mappings); }
  void set_initial_index(uint8_t index) { this->initial_index_ = index; }
  void set_restore_index(bool index) { this->restore_index_ = index; }

 protected:
  void control(size_t index) override;

  TuyaLowPower *parent_;
  bool optimistic_ = false;
  uint8_t select_id_;
  std::vector<uint8_t> mappings_;
  bool is_int_ = false;
  optional<uint8_t> initial_index_{};
  bool restore_index_{false};

  ESPPreferenceObject pref_;
};

}  // namespace esphome::tuya_low_power
