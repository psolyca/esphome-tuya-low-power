# Tuya Low Power Select

The `tuya low power` select platform allows you to create a selection that controls a tuya low power serial component.  
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

On this controller, the datapoint 9 represents the temperature sensor unit, 0=°C and 1=°F.  

Based on this, you can create the select as follows:

```yaml
select:
  - platform: tuya_low_power
    name: "Temperature unit"
    id: "temperature_unit"
    enum_datapoint: 9
    optimistic: true
    options:
      0: C°
      1: F°
    initial_option: C°
```

## Configuration variables

- **enum_datapoint** (**Required**, int): The enum datapoint id number for the select.
  At least one of *enum_datapoint* or *int_datapoint* is required.

- **int_datapoint** (**Required**, int): The int datapoint id number for the select.
  At least one of *enum_datapoint* or *int_datapoint* is required.

- **options** (**Required**, Map[int, str]): Provide a mapping from values (int) of this Select to options (str) of the *enum_datapoint* and vice versa.
  All options and all values have to be unique.

- **optimistic** (*Optional*, boolean): Whether to operate in optimistic mode - when in this mode,
  any command sent to the Select will immediately update the reported state.

- **initial_value** (*Optional*, string): The value to be written at initialization. Must be in `options`.

- **restore_value** (*Optional*, boolean): Saves and loads the state to RTC/Flash. Defaults to `false`.

- All other options from [Select](https://esphome.io/components/select#config-select).

## See Also

- [Tuya Low Power]
- [Select Component](https://esphome.io/components/select/)

[Tuya Low Power]: tuya_low_power_mcu.md

