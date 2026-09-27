# Tuya MCU Low Power 

The `tuya low power` component creates a serial connection to the Tuya MCU for platforms to use.  
This component is based on official [Tuya MCU](https://esphome.io/components/tuya).  

The `tuya low power` serial component requires a UART bus to be configured.  
Put the tuya component in the config and it will list the possible devices for you in the config log.  

```yaml
uart:
  tx_pin: TX1
  rx_pin: RX1
  baud_rate: 9600

tuya_low_power:
```

When [Tuya Low Power] has been properly configured, it will output a list of valid data points to the log (wait for first datapoint handling).  

```text
[21:37:14][C][tuya_low_power:028]: Tuya:
[21:37:14][C][tuya_low_power:045]:   Datapoint 101: enum (value: 4)
[21:37:14][C][tuya_low_power:045]:   Datapoint 102: enum (value: 1)
[21:37:14][C][tuya_low_power:041]:   Datapoint 103: int value (value: 5)
[21:37:14][C][tuya_low_power:039]:   Datapoint 104: switch (value: OFF)
[21:37:14][C][tuya_low_power:041]:   Datapoint 105: int value (value: 229)
[21:37:14][C][tuya_low_power:041]:   Datapoint 106: int value (value: 37)
[21:37:14][C][tuya_low_power:041]:   Datapoint 107: int value (value: 10)
[21:37:14][C][tuya_low_power:041]:   Datapoint 108: int value (value: 35)
[21:37:14][C][tuya_low_power:041]:   Datapoint 109: int value (value: 30)
[21:37:14][C][tuya_low_power:041]:   Datapoint 110: int value (value: 80)
[21:37:14][C][tuya_low_power:039]:   Datapoint 112: switch (value: OFF)
[21:37:14][C][tuya_low_power:039]:   Datapoint 113: switch (value: OFF)
[21:37:14][C][tuya_low_power:039]:   Datapoint 114: switch (value: OFF)
[21:37:14][C][tuya_low_power:045]:   Datapoint 115: enum (value: 4)
[21:37:14][C][tuya_low_power:045]:   Datapoint 116: enum (value: 2)
[21:37:14][C][tuya_low_power:055]:   Product: '{"p":"ymf4oruxqx0xlogp","v":"1.0.3","m":0}'
```


## Configuration variables
- **time_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Some Tuya devices support obtaining local time from ESPHome.
  Specify the ID of the [Time](https://esphome.io/components/time/) which will be used.

- **reset_pin** (*Optional*, [Pin Schema](https://esphome.io/guides/configuration-types#pin-schema)): Special feature to software reset the device.

- **ignore_mcu_update_on_datapoints** (*Optional*, list): A list of datapoints to ignore MCU updates for. Useful for
  certain broken/erratic hardware and debugging.

Automations:

- **on_datapoint_update** (*Optional*): An automation to perform when a Tuya datapoint update is received. See [`on_datapoint_update`](#tuya-on_datapoint_update).

## Tuya Automation

<span id="tuya-on_datapoint_update"></span>

### `on_datapoint_update`

This automation will be triggered when a Tuya datapoint update is received.
A variable `x` is passed to the automation for use in lambdas.
The type of `x` variable is depending on `datapoint_type` configuration variable:

- *raw*: `x` is `std::vector<uint8_t>`
- *string*: `x` is `std::string`
- *bool*: `x` is `bool`
- *int*: `x` is `int`
- *uint*: `x` is `uint32_t`
- *enum*: `x` is `uint8_t`
- *bitmask*: `x` is `uint32_t`
- *any*: `x` is [tuya::TuyaDataPoint](https://api-docs.esphome.io/structesphome_1_1tuya_1_1_tuya_datapoint)


### Configuration variables

- **sensor_datapoint** (**Required**, int): The datapoint id number of the sensor.
- **datapoint_type** (*Optional*, string): The datapoint type one of *raw*, *string*, *bool*, *int*, *uint*, *enum*,
  *bitmask* or *any*.
- See [Automation](https://esphome.io/automations).

# Example
## Time is needed by the device
```yaml
api:

time:
  - platform: homeassistant
    id: hass_time

tuya_low_power:
  time_id: hass_time
```
Do not forget to integrate the device in Home Assistant to be able to receive time from HASS.  
SNTP time could be used. As the time needed to sync time is longer with SNTP, a check is made before sending the command.  
It's not needed to integrate the device in Home Assistant for SNTP.  

## Software reset
```yaml

tuya_low_power:
  reset_pin: P24
```
With [Pin schema](https://esphome.io/guides/configuration-types/#pin-schema) could be used.  
```yaml

tuya_low_power:
  reset_pin:
    number: P24
    inverted: false
```
## Automation
```yaml
tuya:
  on_datapoint_update:
    - sensor_datapoint: 6
      datapoint_type: raw
      then:
        - lambda: |-
            ESP_LOGD("main", "on_datapoint_update %s", format_hex_pretty(x).c_str());
            id(temparature).publish_state(x * 0.1);
    - sensor_datapoint: 7 # sample dp
      datapoint_type: string
      then:
        - lambda: |-
            ESP_LOGD("main", "on_datapoint_update %s", x.c_str());
    - sensor_datapoint: 8 # sample dp
      datapoint_type: bool
      then:
        - lambda: |-
            ESP_LOGD("main", "on_datapoint_update %s", ONOFF(x));
    - sensor_datapoint: 6
      datapoint_type: any # this is optional
      then:
        - lambda: |-
            if (x.type == tuya::TuyaDatapointType::RAW) {
              ESP_LOGD("main", "on_datapoint_update %s", format_hex_pretty(x.value_raw).c_str());
            } else {
              ESP_LOGD("main", "on_datapoint_update %hhu", x.type);
            }
```

# See also

- [Tuya Binary Sensor](tuya_low_power_binary_sensor.md)
- [Tuya Number](tuya_low_power_number.md)
- [Tuya Select](tuya_low_power_select.md)
- [Tuya Sensor](tuya_low_power_sensor.md)
- [Tuya Switch](tuya_low_power_switch.md)
- [Tuya Text Sensor](tuya_low_power_text_sensor.md)

