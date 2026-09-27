# Tuya Low Power Sensor

The `tuya low power` sensor platform allows you to create a sensor that controls a tuya low power serial component.  
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

On this controller, the datapoint 1 represents the temperature in °C which is what we are interested in reading using this platform.

Based on this, you can create the sensor as follows:

```yaml
sensor:
  - platform: tuya_low_power
    name: "Temperature"
    sensor_datapoint: 1
```

As you can use [Sensor](https://esphome.io/components/sensor#config-sensor) configurations, you cans use `lambda`.  
Here, `temperature_unit` is the select component as described in [Tuya Low Power Select](tuya_low_power_select.md).  
You can then print the temperature as Celsius or Farenheit.  
```
sensor:
  - platform: tuya_low_power
    name: "Temperature"
    sensor_datapoint: 1
    filters:
      - lambda: |-
          if (id(temperature_unit).current_option() == "°C") {
            return x/10.0;
          }
          return (x * 9/5) + 32;
```

## Configuration variables

- **sensor_datapoint** (**Required**, int): The datapoint id number of the sensor.

- All other options from [Sensor](https://esphome.io/components/sensor#config-sensor).

## See Also

- [Tuya Low Power]
- [Sensor Component](https://esphome.io/components/sensor/)

[Tuya Low Power]: tuya_low_power_mcu.md

