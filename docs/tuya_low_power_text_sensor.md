# Tuya Low Power Text Sensor

The `tuya low power` text sensor platform allows you to create a text sensor that controls a tuya low power serial component.  
This platform requires [Tuya Low Power] to be configured.  

When [Tuya Low Power] has been properly configured, it will output a list of valid data points to the log (wait for first datapoint handling).  

```text
[15:04:49.015][C][tuya_low_power:075]: Tuya:
[15:04:49.017][C][tuya_low_power:098]:   Datapoint 9: enum (value: 0)
[15:04:49.017][C][tuya_low_power:094]:   Datapoint 1: int value (value: 251)
[15:04:49.024][C][tuya_low_power:094]:   Datapoint 2: int value (value: 62)
[15:04:49.028][C][tuya_low_power:094]:   Datapoint 4: int value (value: 0)
[15:04:49.035][C][tuya_low_power:094]:   Datapoint 6: int value (value: 31)
[15:04:49.040][C][tuya_low_power:094]:   Datapoint 10: int value (value: 50)
[15:04:49.046][C][tuya_low_power:094]:   Datapoint 11: int value (value: -10)
[15:04:49.053][C][tuya_low_power:094]:   Datapoint 12: int value (value: 80)
[15:04:49.059][C][tuya_low_power:094]:   Datapoint 13: int value (value: 20)
[15:04:49.068][C][tuya_low_power:098]:   Datapoint 14: enum (value: 2)
[15:04:49.072][C][tuya_low_power:098]:   Datapoint 15: enum (value: 2)
[15:04:49.077][C][tuya_low_power:094]:   Datapoint 17: int value (value: 1)
[15:04:49.084][C][tuya_low_power:094]:   Datapoint 23: int value (value: -12)
[15:04:49.090][C][tuya_low_power:094]:   Datapoint 24: int value (value: 0)
[15:04:49.102][C][tuya_low_power:131]:   Reset Pin: 24
[15:04:49.102][C][tuya_low_power:106]:   Product: '{"p":"aw3mdykowygrrjm2","v":"1.0.0"}'
```

You can create the text sensor as follows (not in the example):

```yaml
text_sensor:
  - platform: tuya_low_power
    name: "MyTextSensor"
    sensor_datapoint: 18
```

## Configuration variables

- **sensor_datapoint** (**Required**, int): The datapoint id number of the sensor.

- All other options from [Text Sensor](https://esphome.io/components/text_sensor#config-text_sensor).

## See Also

- [Tuya Low Power]
- [Text Sensor Component](https://esphome.io/components/text_sensor/)

[Tuya Low Power]: tuya_low_power_mcu.md

