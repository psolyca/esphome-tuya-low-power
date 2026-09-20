# `ESPHome` Tuya Low Power

[![License][license-shield]][license]
[![ESPHome release][esphome-release-shield]][esphome-release]

[license-shield]: https://img.shields.io/static/v1?label=License&message=ESPHome&color=orange&logo=license
[license]: LICENSE
[esphome-release-shield]: https://img.shields.io/github/release/esphome/esphome.svg
[esphome-release]: https://GitHub.com/esphome/esphome/releases/

ESPHome component for Tuya Low Power devices.  

## 0. Foreword

I made this component for the Avatto WHS20 device I own (MCU & CB3S tuya module).  
This device could be powered by batteries or USB and thus, use low power protocol.  

I was using [OpenBK7231T_App](https://github.com/openshwprojects/OpenBK7231T_App/) but I had some trouble with it.  
Firmware was good but I can not control enough device start.  
The device stop connected to wifi after some module restart.  

Thus, as the device is always USB powered, I modded it to avoid the MCU to control the module power.
I cut the PCB control trace and send it to the module pin P24.  
I then power the module with a direct 3.3 V line.

Thus, this component should work with all low power devices (normal behaviour).  
It is also able to check a pin to reset by software. 

Currently, the device works in wifi mode only. I'd like to be able to connect through BLE.  

## 1. Installation

Use latest [ESPHome](https://esphome.io/) with external components and add this to your `.yaml` definition:

```yaml
external_components:
  - source: github://psolyca/esphome-tuya-low-power

```

## 2. Usage
```yaml
tuya-low-power:
```
### Configuration variables
- **time_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Some Tuya devices support obtaining local time from ESPHome.
  Specify the ID of the [Time](https://esphome.io/components/time/) which will be used.

- **reset_pin** (*Optional*, [Pin Schema](https://esphome.io/guides/configuration-types#pin-schema)): Special feature to software reset the device.

- **ignore_mcu_update_on_datapoints** (*Optional*, list): A list of datapoints to ignore MCU updates for. Useful for
  certain broken/erratic hardware and debugging.

Automations:

- **on_datapoint_update** (*Optional*): An automation to perform when a Tuya datapoint update is received. See [`on_datapoint_update`](#tuya-on_datapoint_update).

### Tuya Automation

<span id="tuya-on_datapoint_update"></span>

#### `on_datapoint_update`

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


#### Configuration variables

- **sensor_datapoint** (**Required**, int): The datapoint id number of the sensor.
- **datapoint_type** (*Optional*, string): The datapoint type one of *raw*, *string*, *bool*, *int*, *uint*, *enum*,
  *bitmask* or *any*.
- See [Automation](https://esphome.io/automations).

## 3. Example
### Time is needed by the device
```yaml
api:

time:
  - platform: homeassistant
    id: hass_time

tuya-low-power:
  time_id: hass_time
```
SNTP time could be used. As the time needed to sync time is longer with SNTP, a check is made before sending the command.  
Do not forget to integrate the device in Home Assistant to be able to receive time from HASS. It's not needed  for SNTP.  

### Software reset
```yaml

tuya-low-power:
  reset_pin: P24
```
With [Pin schema](https://esphome.io/guides/configuration-types/#pin-schema) could be used.  
```yaml

tuya-low-power:
  reset_pin:
    number: P24
    inverted: false
```
### Automation
```yaml
tuya:
  on_datapoint_update:
    - sensor_datapoint: 6
      datapoint_type: raw
      then:
        - lambda: |-
            ESP_LOGD("main", "on_datapoint_update %s", format_hex_pretty(x).c_str());
            id(voltage).publish_state((x[0] << 8 | x[1]) * 0.1);
            id(current).publish_state((x[3] << 8 | x[4]) * 0.001);
            id(power).publish_state((x[6] << 8 | x[7]) * 0.1);
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

## 4. ToDo
* Add command 0x09 aka Send command. Module send commands for settings and control features of the device (online mode!)
* Add command 0x10 aka Obtain DP cache command. MCU request commands for settings and control features of the device (offline mode!)
* Add firmware update capabilities. Module update could be done, MCU could be hard
* Add command 0x0B aka Signal strenght for wifi \[and bluetooth\]
* Use ISR to check reset Pin


## 5. Thanks
Thanks Tuya for their documentation  
[Tuya Low Power protocol](https://developer.tuya.com/en/docs/iot/tuyacloudlowpoweruniversalserialaccessprotocol?id=K95afs9h4tjjh)

Thanks all these people for their work on the protocol:
* [OpenBK7231T_App](https://github.com/openshwprojects/OpenBK7231T_App/)
* [Esphome tuya PIR](https://github.com/brandond/esphome-tuya_pir/)
* [ESPHome Tuya MCU component](https://github.com/esphome/esphome/tree/dev/esphome/components/tuya)
* [ESPHome Tuya MCU documentation](https://esphome.io/components/tuya/)

## 6. Licence
Folowing ESPHome double licence.

