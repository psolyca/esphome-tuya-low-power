#include "tuya_low_power.h"
#include "esphome/components/network/util.h"
#include "esphome/components/safe_mode/safe_mode.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"

#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

#ifdef USE_CAPTIVE_PORTAL
#include "esphome/components/captive_portal/captive_portal.h"
#endif

// Low Powered devices can be shut down after 37s if every thing is ok
// So we mark successful boot after MQTT/API connection cause normal boot is 60 s.

namespace esphome::tuya_low_power {

static const char *const TAG = "tuya_low_power";
static const int COMMAND_DELAY = 10;
static const int RECEIVE_TIMEOUT = 300;
static const int MAX_RETRIES = 5;
// Max bytes to log for datapoint values (larger values are truncated)
static constexpr size_t MAX_DATAPOINT_LOG_BYTES = 256;

void TuyaLowPower::setup() {}

void TuyaLowPower::loop() {
  // Communication is initiated by the network module
  // No other command should be received before
  if (this->init_state_ == TuyaInitState::INIT_HANDSHAKE) {
    ESP_LOGD(TAG, "Device initialisation - Send product query");
    this->send_empty_command_(TuyaCommandType::PRODUCT);
    this->init_state_ = TuyaInitState::HANDSHAKE_DONE;
  } else if (this->init_state_ == TuyaInitState::INIT_NETWORK ||
#ifdef USE_TIME
             (this->init_state_ == TuyaInitState::INIT_CLOUD && this->check_local_time_())) {
#else
             (this->init_state_ == TuyaInitState::INIT_CLOUD)) {
#endif
    this->report_network_status_();
  }
  // Read all available bytes in batches to reduce UART call overhead.
  size_t avail = this->available();
  uint8_t buf[64];
  while (avail > 0) {
    size_t to_read = std::min(avail, sizeof(buf));
    if (!this->read_array(buf, to_read)) {
      break;
    }
    avail -= to_read;

    for (size_t i = 0; i < to_read; i++) {
      this->handle_char_(buf[i]);
    }
  }
  process_command_queue_();

  // Read reset pin and reinit Tuya state
  if (this->reset_pin_ != nullptr) {
    if (this->reset_pin_state_ == 0 && this->reset_pin_->digital_read() == 1) {
      this->init_state_ = TuyaInitState::INIT_HANDSHAKE;
      this->reset_pin_state_ = 1;
    }
    if (this->reset_pin_->digital_read() == 0) {
      this->reset_pin_state_ = 0;
    }
  }
}

void TuyaLowPower::dump_config() {
  ESP_LOGCONFIG(TAG, "Tuya:");
  if (this->init_state_ != TuyaInitState::INIT_DONE) {
    if (this->init_failed_) {
      ESP_LOGCONFIG(TAG, "  Initialization failed. Current init_state: %u", static_cast<uint8_t>(this->init_state_));
    } else {
      ESP_LOGCONFIG(TAG, "  Configuration will be reported when setup is complete. Current init_state: %u",
                    static_cast<uint8_t>(this->init_state_));
    }
    ESP_LOGCONFIG(TAG, "  If no further output is received, confirm that this is a supported Tuya device.");
    return;
  }
  for (auto &info : this->datapoints_) {
    if (info.type == TuyaDatapointType::RAW) {
      char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
      ESP_LOGCONFIG(TAG, "  Datapoint %u: raw (value: %s)", info.id,
                    format_hex_pretty_to(hex_buf, info.value_raw.data(), info.value_raw.size()));
    } else if (info.type == TuyaDatapointType::BOOLEAN) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: switch (value: %s)", info.id, ONOFF(info.value_bool));
    } else if (info.type == TuyaDatapointType::INTEGER) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: int value (value: %d)", info.id, info.value_int);
    } else if (info.type == TuyaDatapointType::STRING) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: string value (value: %s)", info.id, info.value_string.c_str());
    } else if (info.type == TuyaDatapointType::ENUM) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: enum (value: %d)", info.id, info.value_enum);
    } else if (info.type == TuyaDatapointType::BITMASK) {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: bitmask (value: %" PRIx32 ")", info.id, info.value_bitmask);
    } else {
      ESP_LOGCONFIG(TAG, "  Datapoint %u: unknown", info.id);
    }
  }
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  Product: '%s'", this->product_.c_str());
}

bool TuyaLowPower::validate_message_() {
  uint32_t at = this->rx_message_.size() - 1;
  auto *data = &this->rx_message_[0];
  uint8_t new_byte = data[at];

  // Byte 0: HEADER1 (always 0x55)
  if (at == 0)
    return new_byte == 0x55;
  // Byte 1: HEADER2 (always 0xAA)
  if (at == 1)
    return new_byte == 0xAA;

  // Byte 2: VERSION
  // no validation for the following fields:
  uint8_t version = data[2];
  if (at == 2)
    return true;
  // Byte 3: COMMAND
  uint8_t command = data[3];
  if (at == 3)
    return true;

  // Byte 4: LENGTH1
  // Byte 5: LENGTH2
  if (at <= 5) {
    // no validation for these fields
    return true;
  }

  uint16_t length = (uint16_t(data[4]) << 8) | (uint16_t(data[5]));

  // wait until all data is read
  if (at - 6 < length)
    return true;

  // Byte 6+LEN: CHECKSUM - sum of all bytes (including header) modulo 256
  uint8_t rx_checksum = new_byte;
  uint8_t calc_checksum = 0;
  for (uint32_t i = 0; i < 6 + length; i++)
    calc_checksum += data[i];

  if (rx_checksum != calc_checksum) {
    ESP_LOGW(TAG, "Tuya Received invalid message checksum %02X!=%02X", rx_checksum, calc_checksum);
    return false;
  }

  // valid message
  const uint8_t *message_data = data + 6;
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
  ESP_LOGV(TAG, "Received Tuya: CMD=0x%02X VERSION=%u DATA=[%s] INIT_STATE=%u", command, version,
           format_hex_pretty_to(hex_buf, message_data, length), static_cast<uint8_t>(this->init_state_));
#endif
  this->handle_command_(command, version, message_data, length);

  // return false to reset rx buffer
  return false;
}

void TuyaLowPower::handle_char_(uint8_t c) {
  this->rx_message_.push_back(c);
  if (!this->validate_message_()) {
    this->rx_message_.clear();
  } else {
    this->last_rx_char_timestamp_ = millis();
  }
}

void TuyaLowPower::handle_command_(uint8_t command, uint8_t version, const uint8_t *buffer, size_t len) {
  TuyaCommandType command_type = (TuyaCommandType) command;

  if (this->expected_response_.has_value() && this->expected_response_ == command_type) {
    this->expected_response_.reset();
    this->command_queue_.erase(command_queue_.begin());
    this->init_retries_ = 0;
  }

  switch (command_type) {
    case TuyaCommandType::PRODUCT: {
      // check it is a valid string made up of printable characters
      bool valid = true;
      for (size_t i = 0; i < len; i++) {
        if (!std::isprint(buffer[i])) {
          valid = false;
          break;
        }
      }
      if (valid) {
        this->product_ = std::string(reinterpret_cast<const char *>(buffer), len);
      } else {
        this->product_ = R"({"p":"INVALID"})";
      }
      if (this->init_state_ == TuyaInitState::HANDSHAKE_DONE) {
        this->init_state_ = TuyaInitState::INIT_NETWORK;
      }
      break;
    }
    case TuyaCommandType::NETWORK_STATE: {
      if (this->init_state_ == TuyaInitState::INIT_NETWORK &&
          this->network_status_ == TuyaNetworkState::CONNECTED_TO_ROUTER) {
        this->init_state_ = TuyaInitState::INIT_CLOUD;
#ifdef USE_TIME
      } else if (this->init_state_ == TuyaInitState::INIT_CLOUD && this->check_local_time_()) {
#else
      } else if (this->init_state_ == TuyaInitState::INIT_CLOUD) {
#endif
        this->init_state_ = TuyaInitState::INIT_DONE;
      }
      break;
    }
    case TuyaCommandType::NETWORK_SELECT:
    case TuyaCommandType::NETWORK_RESET: {
      const bool is_select = (len >= 1);
      // Send NETWORK_SELECT ACK
      TuyaCommand ack;
      ack.cmd = is_select ? TuyaCommandType::NETWORK_SELECT : TuyaCommandType::NETWORK_RESET;
      ack.payload.clear();
      this->send_command_(ack);
      // Establish pairing mode for correct first NETWORK_STATE byte, STA (0x00) default
      TuyaNetworkState first = TuyaNetworkState::CONF_STA;
      const char *mode_str = "STA";
      if (is_select && buffer[0] == 0x01) {
        first = TuyaNetworkState::CONF_AP;
        mode_str = "AP";
      }
      // Send NETWORK_STATE response, MCU exits pairing mode
      TuyaCommand st;
      st.cmd = TuyaCommandType::NETWORK_STATE;
      st.payload.resize(1);
      st.payload[0] = static_cast<uint8_t>(first);
      this->send_command_(st);
      st.payload[0] = static_cast<uint8_t>(TuyaNetworkState::NOT_CONNECTED);
      this->send_command_(st);
      st.payload[0] = static_cast<uint8_t>(TuyaNetworkState::CONNECTED_TO_ROUTER);
      this->send_command_(st);
      st.payload[0] = static_cast<uint8_t>(TuyaNetworkState::CONNECTED_TO_CLOUD);
      this->send_command_(st);
      ESP_LOGI(TAG, "%s received (%s), replied with NETWORK_STATE confirming connection established",
               is_select ? LOG_STR_LITERAL("NETWORK_SELECT") : LOG_STR_LITERAL("NETWORK_RESET"), mode_str);
      break;
    }
    case TuyaCommandType::DATAPOINT_SYNC:   // 0x05
    case TuyaCommandType::DATAPOINT_ASYNC:  // 0x08
      this->handle_datapoints_(buffer, len, command_type == TuyaCommandType::DATAPOINT_ASYNC ? 1 : 0);

      this->send_command_(TuyaCommand{.cmd = command_type, .payload = std::vector<uint8_t>{0x00}});
      break;
    case TuyaCommandType::DATAPOINT_DELIVER: // 0x09
      break;
    case TuyaCommandType::NETWORK_TEST:
      this->send_command_(
          TuyaCommand{.cmd = TuyaCommandType::NETWORK_TEST, .payload = std::vector<uint8_t>{0x00, 0x00}});
      break;
    case TuyaCommandType::NETWORK_RSSI:
      this->send_command_(
          TuyaCommand{.cmd = TuyaCommandType::NETWORK_RSSI, .payload = std::vector<uint8_t>{get_network_rssi_()}});
      break;
    case TuyaCommandType::LOCAL_TIME:
#ifdef USE_TIME
      if (this->time_id_ != nullptr) {
        this->send_local_time_();

        if (!this->time_sync_callback_registered_) {
          // tuya mcu supports time, so we let them know when our time changed
          this->time_id_->add_on_time_sync_callback([this] { this->send_local_time_(); });
          this->time_sync_callback_registered_ = true;
        }
      } else
#endif
      {
        ESP_LOGW(TAG, "LOCAL_TIME_QUERY is not handled because time is not configured");
      }
      break;
    case TuyaCommandType::DATAPOINT_CACHED:
      this->send_cached_datapoint_command_();
      break;
    default:
      ESP_LOGE(TAG, "Invalid command (0x%02X) received", command);
  }
}

void TuyaLowPower::handle_datapoints_(const uint8_t *buffer, size_t len, uint8_t async, bool cache) {
  if (async == 1) {
    buffer = buffer + 7;
  }
  while (len >= 4) {
    TuyaDatapoint datapoint{};
    datapoint.id = buffer[0];
    datapoint.type = (TuyaDatapointType) buffer[1];
    datapoint.value_uint = 0;

    size_t data_size = (buffer[2] << 8) + buffer[3];
    const uint8_t *data = buffer + 4;
    size_t data_len = len - 4;
    if (data_size > data_len) {
      ESP_LOGW(TAG, "Datapoint %u is truncated and cannot be parsed (%zu > %zu)", datapoint.id, data_size, data_len);
      return;
    }

    datapoint.len = data_size;

    switch (datapoint.type) {
      case TuyaDatapointType::RAW:
        datapoint.value_raw = std::vector<uint8_t>(data, data + data_size);
        {
          char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
          ESP_LOGD(TAG, "Datapoint %u update to %s", datapoint.id,
                   format_hex_pretty_to(hex_buf, datapoint.value_raw.data(), datapoint.value_raw.size()));
        }
        break;
      case TuyaDatapointType::BOOLEAN:
        if (data_size != 1) {
          ESP_LOGW(TAG, "Datapoint %u has bad boolean len %zu", datapoint.id, data_size);
          return;
        }
        datapoint.value_bool = data[0];
        ESP_LOGD(TAG, "Datapoint %u update to %s", datapoint.id, ONOFF(datapoint.value_bool));
        break;
      case TuyaDatapointType::INTEGER:
        if (data_size != 4) {
          ESP_LOGW(TAG, "Datapoint %u has bad integer len %zu", datapoint.id, data_size);
          return;
        }
        datapoint.value_uint = encode_uint32(data[0], data[1], data[2], data[3]);
        ESP_LOGD(TAG, "Datapoint %u update to %d", datapoint.id, datapoint.value_int);
        break;
      case TuyaDatapointType::STRING:
        datapoint.value_string = std::string(reinterpret_cast<const char *>(data), data_size);
        ESP_LOGD(TAG, "Datapoint %u update to %s", datapoint.id, datapoint.value_string.c_str());
        break;
      case TuyaDatapointType::ENUM:
        if (data_size != 1) {
          ESP_LOGW(TAG, "Datapoint %u has bad enum len %zu", datapoint.id, data_size);
          return;
        }
        datapoint.value_enum = data[0];
        ESP_LOGD(TAG, "Datapoint %u update to %d", datapoint.id, datapoint.value_enum);
        break;
      case TuyaDatapointType::BITMASK:
        switch (data_size) {
          case 1:
            datapoint.value_bitmask = encode_uint32(0, 0, 0, data[0]);
            break;
          case 2:
            datapoint.value_bitmask = encode_uint32(0, 0, data[0], data[1]);
            break;
          case 4:
            datapoint.value_bitmask = encode_uint32(data[0], data[1], data[2], data[3]);
            break;
          default:
            ESP_LOGW(TAG, "Datapoint %u has bad bitmask len %zu", datapoint.id, data_size);
            return;
        }
        ESP_LOGD(TAG, "Datapoint %u update to %#08" PRIX32, datapoint.id, datapoint.value_bitmask);
        break;
      default:
        ESP_LOGW(TAG, "Datapoint %u has unknown type %#02hhX", datapoint.id, static_cast<uint8_t>(datapoint.type));
        return;
    }

    len -= data_size + 4;
    buffer = data + data_size;

    // drop update if datapoint is in ignore_mcu_datapoint_update list
    bool skip = false;
    for (auto i : this->ignore_mcu_update_on_datapoints_) {
      if (datapoint.id == i) {
        ESP_LOGV(TAG, "Datapoint %u found in ignore_mcu_update_on_datapoints list, dropping MCU update", datapoint.id);
        skip = true;
        break;
      }
    }
    if (skip)
      continue;

    if (!cache) {
      // Update internal datapoints
      bool found = false;
      for (auto &other : this->datapoints_) {
        if (other.id == datapoint.id) {
          other = datapoint;
          found = true;
        }
      }
      if (!found) {
        this->datapoints_.push_back(datapoint);
      }

      // Run through listeners
      for (auto &listener : this->listeners_) {
        if (listener.datapoint_id == datapoint.id)
          listener.on_datapoint(datapoint);
      }
    } else {
      bool found = false;
      for (auto &other : this->cached_datapoints_) {
        if (other.id == datapoint.id) {
          other = datapoint;
          found = true;
        }
      }
      if (!found) {
        this->cached_datapoints_.push_back(datapoint);
      }
    }
  }
}

void TuyaLowPower::send_raw_command_(TuyaCommand command) {
  uint8_t len_hi = (uint8_t) (command.payload.size() >> 8);
  uint8_t len_lo = (uint8_t) (command.payload.size() & 0xFF);
  uint8_t version = 0;

  this->last_command_timestamp_ = millis();
  switch (command.cmd) {
    case TuyaCommandType::PRODUCT:
      this->expected_response_ = TuyaCommandType::PRODUCT;
      break;
    case TuyaCommandType::NETWORK_STATE:
      this->expected_response_ = TuyaCommandType::NETWORK_STATE;
      break;
    case TuyaCommandType::DATAPOINT_DELIVER:
      this->expected_response_ = TuyaCommandType::DATAPOINT_DELIVER;
      break;
    default:
      break;
  }

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
  ESP_LOGV(TAG, "Sending Tuya: CMD=0x%02X VERSION=%u DATA=[%s] INIT_STATE=%u", static_cast<uint8_t>(command.cmd),
           version, format_hex_pretty_to(hex_buf, command.payload.data(), command.payload.size()),
           static_cast<uint8_t>(this->init_state_));
#endif

  this->write_array({0x55, 0xAA, version, (uint8_t) command.cmd, len_hi, len_lo});
  if (!command.payload.empty())
    this->write_array(command.payload.data(), command.payload.size());

  uint8_t checksum = 0x55 + 0xAA + (uint8_t) command.cmd + len_hi + len_lo;
  for (auto &data : command.payload)
    checksum += data;
  this->write_byte(checksum);
}

void TuyaLowPower::process_command_queue_() {
  uint32_t now = millis();
  uint32_t delay = now - this->last_command_timestamp_;

  if (now - this->last_rx_char_timestamp_ > RECEIVE_TIMEOUT) {
    this->rx_message_.clear();
  }

  if (this->expected_response_.has_value() && delay > RECEIVE_TIMEOUT) {
    this->expected_response_.reset();
    if (init_state_ != TuyaInitState::INIT_DONE) {
      if (++this->init_retries_ >= MAX_RETRIES) {
        this->init_failed_ = true;
        ESP_LOGE(TAG, "Initialization failed at init_state %u", static_cast<uint8_t>(this->init_state_));
        this->command_queue_.erase(command_queue_.begin());
        this->init_retries_ = 0;
      }
    } else {
      this->command_queue_.erase(command_queue_.begin());
    }
  }

  // Left check of delay since last command in case there's ever a command sent by calling send_raw_command_ directly
  if (delay > COMMAND_DELAY && !this->command_queue_.empty() && this->rx_message_.empty() &&
      !this->expected_response_.has_value()) {
    this->send_raw_command_(command_queue_.front());
    if (!this->expected_response_.has_value())
      this->command_queue_.erase(command_queue_.begin());
  }
}

void TuyaLowPower::send_command_(const TuyaCommand &command) {
  command_queue_.push_back(command);
  process_command_queue_();
}

void TuyaLowPower::send_empty_command_(TuyaCommandType command) {
  send_command_(TuyaCommand{.cmd = command, .payload = std::vector<uint8_t>{}});
}

TuyaNetworkState TuyaLowPower::get_network_status_code_() {
  TuyaNetworkState status = TuyaNetworkState::NOT_CONNECTED;  // 0x02

  if (network::is_connected()) {
    status = TuyaNetworkState::CONNECTED_TO_ROUTER;  // 0x03

    if (remote_is_connected() && this->init_state_ == TuyaInitState::INIT_CLOUD) {
      status = TuyaNetworkState::CONNECTED_TO_CLOUD;  // 0x04
    }
  } else {
#ifdef USE_CAPTIVE_PORTAL
    if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active()) {
      status = TuyaNetworkState::CONF_AP;
    }
#endif
  };

  return status;
}

uint8_t TuyaLowPower::get_network_rssi_() {  // ToDo, check how-to with bluetooth
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr)
    return wifi::global_wifi_component->wifi_rssi();
#endif

  return 0;
}

void TuyaLowPower::report_network_status_() {
  TuyaNetworkState status = this->get_network_status_code_();

  if (status == this->network_status_) {
    return;
  }

  this->network_status_ = status;
  this->send_command_(TuyaCommand{.cmd = TuyaCommandType::NETWORK_STATE,
                                  .payload = std::vector<uint8_t>{static_cast<uint8_t>(status)}});
}

#ifdef USE_TIME
bool TuyaLowPower::check_local_time_() {
  ESPTime local_time = this->time_id_->now();
  bool is_valid = local_time.is_valid();
  return is_valid;
}
void TuyaLowPower::send_local_time_() {
  std::vector<uint8_t> payload;
  ESPTime now = this->time_id_->now();
  if (now.is_valid()) {
    uint8_t year = now.year - 2000;
    uint8_t month = now.month;
    uint8_t day_of_month = now.day_of_month;
    uint8_t hour = now.hour;
    uint8_t minute = now.minute;
    uint8_t second = now.second;
    // Tuya days starts from Monday, esphome uses Sunday as day 1
    uint8_t day_of_week = now.day_of_week - 1;
    if (day_of_week == 0) {
      day_of_week = 7;
    }
    ESP_LOGD(TAG, "Sending local time");
    payload = std::vector<uint8_t>{0x01, year, month, day_of_month, hour, minute, second, day_of_week};
    this->send_command_(TuyaCommand{.cmd = TuyaCommandType::LOCAL_TIME, .payload = payload});
  }
}
#endif

void TuyaLowPower::set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value) {
  this->set_raw_datapoint_value_(datapoint_id, value, false);
}

void TuyaLowPower::set_boolean_datapoint_value(uint8_t datapoint_id, bool value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BOOLEAN, value, 1, false);
}

void TuyaLowPower::set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::INTEGER, value, 4, false);
}

void TuyaLowPower::set_string_datapoint_value(uint8_t datapoint_id, const std::string &value) {
  this->set_string_datapoint_value_(datapoint_id, value, false);
}

void TuyaLowPower::set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::ENUM, value, 1, false);
}

void TuyaLowPower::set_bitmask_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BITMASK, value, length, false);
}

void TuyaLowPower::force_set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value) {
  this->set_raw_datapoint_value_(datapoint_id, value, true);
}

void TuyaLowPower::force_set_boolean_datapoint_value(uint8_t datapoint_id, bool value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BOOLEAN, value, 1, true);
}

void TuyaLowPower::force_set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::INTEGER, value, 4, true);
}

void TuyaLowPower::force_set_string_datapoint_value(uint8_t datapoint_id, const std::string &value) {
  this->set_string_datapoint_value_(datapoint_id, value, true);
}

void TuyaLowPower::force_set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::ENUM, value, 1, true);
}

void TuyaLowPower::force_set_bitmask_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length) {
  this->set_numeric_datapoint_value_(datapoint_id, TuyaDatapointType::BITMASK, value, length, true);
}

optional<TuyaDatapoint> TuyaLowPower::get_datapoint_(uint8_t datapoint_id) {
  for (auto &datapoint : this->datapoints_) {
    if (datapoint.id == datapoint_id)
      return datapoint;
  }
  return {};
}

void TuyaLowPower::set_numeric_datapoint_value_(uint8_t datapoint_id, TuyaDatapointType datapoint_type,
                                                const uint32_t value, uint8_t length, bool forced) {
  ESP_LOGD(TAG, "Setting datapoint %u to %" PRIu32, datapoint_id, value);
  optional<TuyaDatapoint> datapoint = this->get_datapoint_(datapoint_id);
  if (!datapoint.has_value()) {
    ESP_LOGW(TAG, "Setting unknown datapoint %u", datapoint_id);
  } else if (datapoint->type != datapoint_type) {
    ESP_LOGE(TAG, "Attempt to set datapoint %u with incorrect type", datapoint_id);
    return;
  } else if (!forced && datapoint->value_uint == value) {
    ESP_LOGV(TAG, "Not sending unchanged value");
    return;
  }

  std::vector<uint8_t> data;
  switch (length) {
    case 4:
      data.push_back(value >> 24);
      data.push_back(value >> 16);
      [[fallthrough]];
    case 2:
      data.push_back(value >> 8);
      [[fallthrough]];
    case 1:
      data.push_back(value >> 0);
      break;
    default:
      ESP_LOGE(TAG, "Unexpected datapoint length %u", length);
      return;
  }
  this->send_datapoint_command_(datapoint_id, datapoint_type, data);
}

void TuyaLowPower::set_raw_datapoint_value_(uint8_t datapoint_id, const std::vector<uint8_t> &value, bool forced) {
  char hex_buf[format_hex_pretty_size(MAX_DATAPOINT_LOG_BYTES)];
  ESP_LOGD(TAG, "Setting datapoint %u to %s", datapoint_id, format_hex_pretty_to(hex_buf, value.data(), value.size()));
  optional<TuyaDatapoint> datapoint = this->get_datapoint_(datapoint_id);
  if (!datapoint.has_value()) {
    ESP_LOGW(TAG, "Setting unknown datapoint %u", datapoint_id);
  } else if (datapoint->type != TuyaDatapointType::RAW) {
    ESP_LOGE(TAG, "Attempt to set datapoint %u with incorrect type", datapoint_id);
    return;
  } else if (!forced && datapoint->value_raw == value) {
    ESP_LOGV(TAG, "Not sending unchanged value");
    return;
  }
  this->send_datapoint_command_(datapoint_id, TuyaDatapointType::RAW, value);
}

void TuyaLowPower::set_string_datapoint_value_(uint8_t datapoint_id, const std::string &value, bool forced) {
  ESP_LOGD(TAG, "Setting datapoint %u to %s", datapoint_id, value.c_str());
  optional<TuyaDatapoint> datapoint = this->get_datapoint_(datapoint_id);
  if (!datapoint.has_value()) {
    ESP_LOGW(TAG, "Setting unknown datapoint %u", datapoint_id);
  } else if (datapoint->type != TuyaDatapointType::STRING) {
    ESP_LOGE(TAG, "Attempt to set datapoint %u with incorrect type", datapoint_id);
    return;
  } else if (!forced && datapoint->value_string == value) {
    ESP_LOGV(TAG, "Not sending unchanged value");
    return;
  }
  std::vector<uint8_t> data;
  for (char const &c : value) {
    data.push_back(c);
  }
  this->send_datapoint_command_(datapoint_id, TuyaDatapointType::STRING, data);
}

void TuyaLowPower::send_datapoint_command_(uint8_t datapoint_id, TuyaDatapointType datapoint_type,
                                           std::vector<uint8_t> data) {
  std::vector<uint8_t> buffer;
  buffer.push_back(datapoint_id);
  buffer.push_back(static_cast<uint8_t>(datapoint_type));
  buffer.push_back(data.size() >> 8);
  buffer.push_back(data.size() >> 0);
  buffer.insert(buffer.end(), data.begin(), data.end());

  // By default cache datapoint one by one
  // Updating all datapoints could be possible
  /*if (this->cached_datapoints_.empty())
    this->cached_datapoints_ = this->datapoints_;*/
  this->handle_datapoints_(buffer.data(), buffer.size(), 0, true); //Cache datapoints

  this->send_command_(TuyaCommand{.cmd = TuyaCommandType::DATAPOINT_DELIVER, .payload = buffer});
}

void TuyaLowPower::send_cached_datapoint_command_() {
  std::vector<uint8_t> buffer;
  buffer.push_back(0x01); //Success
  buffer.push_back(0x00); //Number of dp
  if (!this->cached_datapoints_.empty()) {
    uint8_t number_datapoint = 0x00;
    for (auto &datapoint : this->cached_datapoints_) {
      buffer.push_back(datapoint.id);
      buffer.push_back(static_cast<uint8_t>(datapoint.type));
      buffer.push_back(datapoint.len >> 8);
      buffer.push_back(datapoint.len >> 0);
      std::vector<uint8_t> data;
      switch (datapoint.type) {
        case TuyaDatapointType::RAW:
          buffer.insert(buffer.end(), datapoint.value_raw.begin(), datapoint.value_raw.end());
          break;
        case TuyaDatapointType::BOOLEAN:
          buffer.insert(buffer.end(), datapoint.value_bool);
          break;
        case TuyaDatapointType::INTEGER:
          data.push_back(datapoint.value_uint >> 24);
          data.push_back(datapoint.value_uint >> 16);
          data.push_back(datapoint.value_uint >> 8);
          data.push_back(datapoint.value_uint >> 0);
          buffer.insert(buffer.end(), data.begin() , data.end());
          break;
        case TuyaDatapointType::STRING:
          buffer.insert(buffer.end(), datapoint.value_string.begin(), datapoint.value_string.end());
          break;
        case TuyaDatapointType::ENUM:
          buffer.insert(buffer.end(), datapoint.value_enum);
          break;
        case TuyaDatapointType::BITMASK:
          data.push_back(datapoint.value_bitmask >> 24);
          data.push_back(datapoint.value_bitmask >> 16);
          data.push_back(datapoint.value_bitmask >> 8);
          data.push_back(datapoint.value_bitmask >> 0);
          buffer.insert(buffer.end(), data.begin() , data.end());
          break;
        default:
          return;
      }
      number_datapoint += 0x01;
    }
    buffer.at(1) = number_datapoint;
    this->cached_datapoints_.clear();
  }

  this->send_command_(TuyaCommand{.cmd = TuyaCommandType::DATAPOINT_CACHED, .payload = buffer});
}

void TuyaLowPower::register_listener(uint8_t datapoint_id, const std::function<void(TuyaDatapoint)> &func) {
  auto listener = TuyaDatapointListener{
      .datapoint_id = datapoint_id,
      .on_datapoint = func,
  };
  this->listeners_.push_back(listener);

  // Run through existing datapoints
  for (auto &datapoint : this->datapoints_) {
    if (datapoint.id == datapoint_id)
      func(datapoint);
  }
}

TuyaInitState TuyaLowPower::get_init_state() { return this->init_state_; }

}  // namespace esphome::tuya_low_power
