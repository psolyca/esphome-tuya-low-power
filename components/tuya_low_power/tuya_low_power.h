#pragma once

#include <cinttypes>
#include <cstdint>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/time.h"
#endif

namespace esphome::tuya_low_power {

enum class TuyaDatapointType : uint8_t {
  RAW = 0x00,      // variable length
  BOOLEAN = 0x01,  // 1 byte (0/1)
  INTEGER = 0x02,  // 4 byte
  STRING = 0x03,   // variable length
  ENUM = 0x04,     // 1 byte
  BITMASK = 0x05,  // 1/2/4 bytes
};

struct TuyaDatapoint {
  uint8_t id;
  TuyaDatapointType type;
  size_t len;
  union {
    bool value_bool;
    int value_int;
    uint32_t value_uint;
    uint8_t value_enum;
    uint32_t value_bitmask;
  };
  std::string value_string;
  std::vector<uint8_t> value_raw;
};

struct TuyaDatapointListener {
  uint8_t datapoint_id;
  std::function<void(TuyaDatapoint)> on_datapoint;
};

enum class TuyaCommandType : uint8_t {
  PRODUCT = 0x01,        // From module
  NETWORK_STATE = 0x02,  // From module
  NETWORK_RESET = 0x03,  // From MCU
  NETWORK_SELECT = 0x04,
  DATAPOINT_SYNC = 0x05,
  LOCAL_TIME = 0x06,
  NETWORK_TEST = 0x07,
  DATAPOINT_ASYNC = 0x08,
  DATAPOINT_DELIVER = 0x09,  // From module
  MODULE_UPGRADE = 0x0A,     // From MCU
  NETWORK_RSSI = 0x0B,
  MCU_UPGRADE = 0x0C,
  MCU_UPGRADE_SIZE = 0x0D,
  MCU_UPGRADE_PACKET = 0x0E,
  DATAPOINT_CACHED = 0x10,
};

enum class TuyaNetworkState : uint8_t {
  CONF_STA = 0x00,
  CONF_AP,
  NOT_CONNECTED,
  CONNECTED_TO_ROUTER,
  CONNECTED_TO_CLOUD,
};

enum class TuyaInitState : uint8_t {
  INIT_HANDSHAKE = 0x00,
  HANDSHAKE_DONE,
  INIT_NETWORK,
  INIT_CLOUD,
  INIT_DONE,
};

struct TuyaCommand {
  TuyaCommandType cmd;
  std::vector<uint8_t> payload;
};

class TuyaLowPower final : public Component, public uart::UARTDevice {
 public:
  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  void register_listener(uint8_t datapoint_id, const std::function<void(TuyaDatapoint)> &func);
  void set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value);
  void set_boolean_datapoint_value(uint8_t datapoint_id, bool value);
  void set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value);
  void set_reset_pin(InternalGPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }
  void set_string_datapoint_value(uint8_t datapoint_id, const std::string &value);
  void set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value);
  void set_bitmask_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length);
  void force_set_raw_datapoint_value(uint8_t datapoint_id, const std::vector<uint8_t> &value);
  void force_set_boolean_datapoint_value(uint8_t datapoint_id, bool value);
  void force_set_integer_datapoint_value(uint8_t datapoint_id, uint32_t value);
  void force_set_string_datapoint_value(uint8_t datapoint_id, const std::string &value);
  void force_set_enum_datapoint_value(uint8_t datapoint_id, uint8_t value);
  void force_set_bitmask_datapoint_value(uint8_t datapoint_id, uint32_t value, uint8_t length);
  TuyaInitState get_init_state();
#ifdef USE_TIME
  void set_time_id(time::RealTimeClock *time_id) { this->time_id_ = time_id; }
#endif
  void add_ignore_mcu_update_on_datapoints(uint8_t ignore_mcu_update_on_datapoints) {
    this->ignore_mcu_update_on_datapoints_.push_back(ignore_mcu_update_on_datapoints);
  }
  template<typename F> void add_on_initialized_callback(F &&callback) {
    this->initialized_callback_.add(std::forward<F>(callback));
  }

 protected:
  void handle_char_(uint8_t c);
  void handle_datapoints_(const uint8_t *buffer, size_t len, uint8_t async);
  optional<TuyaDatapoint> get_datapoint_(uint8_t datapoint_id);
  bool validate_message_();

  void handle_command_(uint8_t command, uint8_t version, const uint8_t *buffer, size_t len);
  void send_raw_command_(TuyaCommand command);
  void process_command_queue_();
  void send_command_(const TuyaCommand &command);
  void send_empty_command_(TuyaCommandType command);
  void set_numeric_datapoint_value_(uint8_t datapoint_id, TuyaDatapointType datapoint_type, uint32_t value,
                                    uint8_t length, bool forced);
  void set_string_datapoint_value_(uint8_t datapoint_id, const std::string &value, bool forced);
  void set_raw_datapoint_value_(uint8_t datapoint_id, const std::vector<uint8_t> &value, bool forced);
  void send_datapoint_command_(uint8_t datapoint_id, TuyaDatapointType datapoint_type, std::vector<uint8_t> data);
  void set_reset_pin_();
  void report_network_status_();
  TuyaNetworkState get_network_status_code_();
  uint8_t get_network_rssi_();

#ifdef USE_TIME
  bool check_local_time_();
  void send_local_time_();
  void send_gmt_time_();
  time::RealTimeClock *time_id_{nullptr};
  bool time_sync_callback_registered_{false};
  bool gmt_time_sync_callback_registered_{false};
#endif
  TuyaInitState init_state_ = TuyaInitState::INIT_HANDSHAKE;
  bool init_failed_{false};
  int init_retries_{0};
  uint8_t protocol_version_ = -1;
  InternalGPIOPin *reset_pin_{nullptr};
  bool reset_pin_state_{false};
  uint32_t last_command_timestamp_ = 0;
  uint32_t last_rx_char_timestamp_ = 0;
  std::string product_;
  std::vector<TuyaDatapointListener> listeners_;
  std::vector<TuyaDatapoint> datapoints_;
  std::vector<uint8_t> rx_message_;
  std::vector<uint8_t> ignore_mcu_update_on_datapoints_{};
  std::vector<TuyaCommand> command_queue_;
  optional<TuyaCommandType> expected_response_{};
  TuyaNetworkState network_status_ = TuyaNetworkState::CONF_STA;
  CallbackManager<void()> initialized_callback_{};
};

}  // namespace esphome::tuya_low_power
