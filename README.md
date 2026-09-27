# `ESPHome` Tuya Low Power

[![License][license-shield]][license]
[![ESPHome release][esphome-release-shield]][esphome-release]

[license-shield]: https://img.shields.io/static/v1?label=License&message=ESPHome&color=orange&logo=license
[license]: LICENSE
[esphome-release-shield]: https://img.shields.io/github/release/esphome/esphome.svg
[esphome-release]: https://GitHub.com/esphome/esphome/releases/

ESPHome component for Tuya Low Power devices.  
![esphome-logo](docs/img/esphome.png) ![tuya-logo](docs/img/tuya.png)  

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

## 2. Basic usage

```yaml
tuya_low_power:
```
### Configuration variables

- **time_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Some Tuya devices support obtaining local time from ESPHome.
  Specify the ID of the [Time](https://esphome.io/components/time/) which will be used.

- **reset_pin** (*Optional*, [Pin Schema](https://esphome.io/guides/configuration-types#pin-schema)): Special feature to software reset the device.

- **ignore_mcu_update_on_datapoints** (*Optional*, list): A list of datapoints to ignore MCU updates for. Useful for
  certain broken/erratic hardware and debugging.

Automations:

- **on_datapoint_update** (*Optional*): An automation to perform when a Tuya datapoint update is received. See [`on_datapoint_update`](#tuya-on_datapoint_update).

## 3. Components and documentation

- [Tuya MCU Low Power](docs/tuya_low_power_mcu.md)
- [Tuya Binary Sensor](docs/tuya_low_power_binary_sensor.md)
- [Tuya Number](docs/tuya_low_power_number.md)
- [Tuya Select](docs/tuya_low_power_select.md)
- [Tuya Sensor](docs/tuya_low_power_sensor.md)
- [Tuya Switch](docs/tuya_low_power_switch.md)
- [Tuya Text Sensor](docs/tuya_low_power_text_sensor.md)

## 4. ToDo

* ~~Add command 0x09 aka Send command. Module send commands for settings and control features of the device (online mode!)~~
* ~~Add command 0x10 aka Obtain DP cache command. MCU request commands for settings and control features of the device (offline mode!)~~
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

Folowing [ESPHome double licence](LICENSE).


