# Tuya Low Power Number

The `tuya low power` number platform allows you to create a number that controls a tuya low power serial component.  
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

The `tuya low power` number platform can be used to control all of the integer and enum datapoints.  

On this controller, datapoint 2 represents the humidity, with valid values being between 0% and 100%.

Based on this, you can create a number as follows:

```yaml
- platform: tuya_low_power
  name: "Humidity"
  number_datapoint: 2
  min_value: 0
  max_value: 100
  step: 1
```

The value for `multiply` is used as the scaling factor for the Number. All numbers in Tuya are integers, so a scaling factor is sometimes needed to convert the Tuya reported value into floating point.

For instance, in the example above, datapoint 23 represent the temperature calibration from -99 to 99 with a scaling of 0.1.  
By setting `multiply` to 10, on the Tuya side (not visible to the user) the number will be reported as an integer from -9.9 to 9.9.  
The following configuration could be used:

```yaml
- platform: tuya_low_power
  name: "Temperature calibration"
  number_datapoint: 23
  min_value: -9.9
  max_value: 9.9
  multiply: 10
  step: 0.1
```

## Configuration variables

- **number_datapoint** (**Required**, int): The datapoint id number of the number.

- **min_value** (**Required**, float): The minimum value this number can be.

- **max_value** (**Required**, float): The maximum value this number can be.

- **step** (*Optional*, float): The granularity with which the number can be set. Defaults to 1.

- **multiply** (*Optional*, float): multiply the new value with this factor before sending the requests.

- **initial_value** (*Optional*, float): The value to be written at initialization. Must be between `min_value` and `max_value`.

- **restore_value** (*Optional*, boolean): Saves and loads the state to RTC/Flash. Defaults to `false`.

- All other options from [Number](https://esphome.io/components/number#config-number).

## See Also

- [Tuya Low Power]
- [Number Component](https://esphome.io/components/number/)

[Tuya Low Power]: tuya_low_power_mcu.md

