#include "esphome/core/log.h"
#include "tuya_select.h"

namespace esphome::tuya_low_power {

static const char *const TAG = "tuya_low_power.select";

void TuyaLPSelect::setup() {
  if (this->restore_index_) {
    this->pref_ = this->make_entity_preference<size_t>();
  }

  this->parent_->register_listener(this->select_id_, [this](const TuyaDatapoint &datapoint) {
    uint8_t enum_value = datapoint.value_enum;
    ESP_LOGV(TAG, "MCU reported select %u value %u", this->select_id_, enum_value);
    auto mappings = this->mappings_;
    auto it = std::find(mappings.cbegin(), mappings.cend(), enum_value);
    if (it == mappings.end()) {
      ESP_LOGW(TAG, "Invalid value %u", enum_value);
      return;
    }
    size_t mapping_idx = std::distance(mappings.cbegin(), it);
    this->publish_state(mapping_idx);
    if (this->restore_index_)
      this->pref_.save(&mapping_idx);
  });

  this->parent_->add_on_initialized_callback([this] {
      size_t mapping_idx;
      if (!this->restore_index_) {
        if (this->initial_index_) {
          mapping_idx = *this->initial_index_;
        } else {
          return;
        }
      } else {
        if (!this->pref_.load(&mapping_idx)) {
          if (this->initial_index_) {
            mapping_idx = *this->initial_index_;
          } else {
            ESP_LOGW(TAG, "Failed to restore and there is no initial value defined.");
          }
        }
      }

      this->control(mapping_idx);
  });
  
}

void TuyaLPSelect::control(size_t index) {
  if (this->optimistic_)
    this->publish_state(index);

  uint8_t mapping = this->mappings_.at(index);
  ESP_LOGV(TAG, "Setting %u datapoint value to %u:%s", this->select_id_, mapping, this->option_at(index));
  if (this->is_int_) {
    this->parent_->set_integer_datapoint_value(this->select_id_, mapping);
  } else {
    this->parent_->set_enum_datapoint_value(this->select_id_, mapping);
  }
  if (this->restore_index_)
    this->pref_.save(&index);
}

void TuyaLPSelect::dump_config() {
  LOG_SELECT("", "Tuya Select", this);
  ESP_LOGCONFIG(TAG,
                "  Select has datapoint ID %u\n"
                "  Data type: %s\n"
                "  Options are:",
                this->select_id_, this->is_int_ ? LOG_STR_LITERAL("int") : LOG_STR_LITERAL("enum"));
  const auto &options = this->traits.get_options();
  for (size_t i = 0; i < this->mappings_.size(); i++) {
    ESP_LOGCONFIG(TAG, "    %i: %s", this->mappings_.at(i), options.at(i));
  }

  if (this->initial_index_) {
    ESP_LOGCONFIG(TAG, "  Initial Index: %f", *this->initial_index_);
  }

  ESP_LOGCONFIG(TAG, "  Restore Index: %s", YESNO(this->restore_index_));
}

}  // namespace esphome::tuya_low_power
