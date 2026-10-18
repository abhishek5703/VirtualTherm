# VirtualTherm test plan

The automated tests are run with `ctest --test-dir build --output-on-failure` (add `-C Release` on a multi-configuration Windows generator).

| Test area | Input / scenario | Expected behavior |
|---|---|---|
| Fan thresholds | Temperatures move upward and downward across mode thresholds | Correct fan mode; separate rising/falling thresholds prevent rapid switching |
| Warning status | Temperature enters elevated range | WARNING status is reported |
| Critical latch | Temperature reaches critical threshold, then partially cools | HIGH fan stays commanded until raw and filtered temperatures reach the lower reset threshold |
| Sensor disconnect | Reading becomes invalid | Controller immediately commands HIGH and reports SENSOR_FAULT |
| Recovery validation | Sensor becomes valid again | HIGH remains active until three consecutive valid samples have arrived |
| EMA filter | A step change in reading | Output moves smoothly toward the new reading |
| Filter reset | Invalid sample followed by a valid sample | Stale history is discarded; next valid reading initializes the filter |
| Thermal model | High workload followed by fan cooling | Temperature rises under load, then drops under cooling and never falls below ambient |
| Overheat end-to-end | `overheat` scenario | Controller reaches HIGH and CRITICAL states |
| Fault end-to-end | `sensor-fault` scenario | Exactly ten invalid samples are injected, with HIGH fan throughout the fault interval |
| Repeatability | Same noisy-sensor timestamp is evaluated more than once | Noise sample is identical, making scenario results reproducible |

## Manual smoke tests

```bash
virtualtherm --scenario normal --steps 150 --csv examples/normal_run.csv
virtualtherm --scenario overheat --steps 150 --csv examples/overheat_run.csv
virtualtherm --scenario sensor-fault --steps 90 --csv examples/sensor_fault_run.csv
virtualtherm --scenario workload-spike --steps 150 --csv examples/workload_spike_run.csv
virtualtherm --scenario noisy-sensor --steps 150 --csv examples/noisy_sensor_run.csv
```

Check that each command completes, produces a summary, and writes one CSV row per simulated step plus the header. Confirm `CRITICAL` appears in the overheating run and `SENSOR_FAULT` and recovery events appear in the sensor-fault run.
