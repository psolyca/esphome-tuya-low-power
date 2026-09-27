# Tuya Low Power Switch

The `tuya low power` switch platform allows you to create a switch that controls a tuya low power serial component.  
This platform requires [Tuya Low Power] to be configured.  

When [Tuya Low Power] has been properly configured, it will output a list of valid data points to the log (wait for first datapoint handling).  

```text
[13:46:01][C][tuya:023]: Tuya:
[13:46:01][C][tuya:032]:   Datapoint 1: switch (value: OFF)
[13:46:01][C][tuya:032]:   Datapoint 2: switch (value: OFF)
[13:46:01][C][tuya:034]:   Datapoint 3: int value (value: 19)
[13:46:01][C][tuya:034]:   Datapoint 4: int value (value: 17)
[13:46:01][C][tuya:034]:   Datapoint 5: int value (value: 0)
[13:46:01][C][tuya:036]:   Datapoint 7: enum (value: 1)
[13:46:01][C][tuya:046]:   Product: '{"p":"ynjanlglr4qa6dxf","v":"1.0.0","m":0}'
```

On this controller, the datapoint 2 represents the child lock switch setting which is what we are interested in controlling using this platform.

Based on this, you can create the switch as follows:

```yaml
switch:
  - platform: tuya_low_power
    name: "MySwitch"
    switch_datapoint: 2
```

## Configuration variables

- **switch_datapoint** (**Required**, int): The datapoint id number of the switch.

- All other options from [Switch](https://esphome.io/components/switch#config-switch).

## See Also

- [Tuya Low Power]
- [Switch Component](https://esphome.io/components/switch/)

[Tuya Low Power]: tuya_low_power_mcu.md
