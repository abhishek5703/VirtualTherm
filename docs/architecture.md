# VirtualTherm architecture

VirtualTherm is a deterministic software simulation of an embedded cooling controller. It runs on a desktop computer and does not communicate with physical hardware.

```text
SimulationScenario
  |-- normalized workload profile
  |-- sensor disconnect window
  `-- deterministic sensor noise
             |
             v
TemperatureSimulator <------------------------------+
             |                                      |
             v                                      |
VirtualTemperatureSensor                            |
             | raw SensorReading                    |
             v                                      |
TemperatureFilter (exponential moving average)      |
             | raw + filtered reading               |
             v                                      |
FanController                                       |
  |-- input plausibility checks                     |
  |-- fan state machine with hysteresis             |
  |-- critical-temperature latch                   |
  `-- sensor-fault recovery counter                 |
             |                                      |
             +-- fan command + estimated RPM -------+
             `-- status/events --> console + CSV
```

## Modules

- `SimulationScenario`: defines the workload timeline, sensor-fault interval, and deterministic noise profile.
- `TemperatureSimulator`: first-order thermal model. Workload adds heat; ambient and fan-dependent cooling remove heat. The model is illustrative and has not been calibrated against real equipment.
- `VirtualTemperatureSensor`: converts the model temperature into a valid reading or simulates a disconnect. Noise is deterministic so runs are reproducible.
- `TemperatureFilter`: exponential moving average (EMA). The first valid sample initializes the filter. Invalid input resets its history so old measurements are not silently reused after a fault.
- `FanController`: verifies readings, maps temperature to fan output with hysteresis, latches critical temperature, and requires three consecutive valid samples before sensor-fault recovery.
- `CsvLogger`: writes one row per simulated step with raw and filtered readings, simulated plant temperature, fan command, status and events.
- `main.cpp`: parses command-line arguments, coordinates the simulation, prints live data and summarizes each run.

## Control rules

### Fan command

| Current mode | Transition rule |
|---|---|
| OFF | LOW at 40°C; MEDIUM at 55°C; HIGH at 70°C |
| LOW | OFF at 37°C or below; MEDIUM at 55°C; HIGH at 70°C |
| MEDIUM | OFF at 37°C or below; LOW at 52°C or below; HIGH at 70°C |
| HIGH | OFF at 37°C or below; LOW at 52°C or below; MEDIUM at 67°C or below |

The controller uses the filtered temperature for ordinary mode selection. A raw reading at or above the critical threshold can force HIGH immediately so that the moving average does not delay the critical response.

### Status and fault handling

- **NORMAL:** filtered temperature below 55°C and no active fault latch.
- **WARNING:** temperature is elevated but no critical latch is active.
- **CRITICAL:** raw or filtered temperature reaches 78°C. HIGH fan is commanded and the status stays latched until both raw and filtered readings fall to 72°C or below.
- **SENSOR_FAULT:** sensor reading is invalid, non-finite, or outside the accepted range. HIGH fan is commanded immediately. Three consecutive valid readings are required before normal control resumes.

These thresholds are chosen for this demo only. They are not certified protection limits for real hardware.

### Modelled fan output

| Mode | Command | Estimated RPM |
|---|---:|---:|
| OFF | 0% | 0 |
| LOW | 35% | 800 |
| MEDIUM | 65% | 1400 |
| HIGH | 100% | 2000 |

RPM values are fixed simulation estimates, not values measured from a real fan.

## Design trade-offs worth discussing

- **Filtering vs. responsiveness:** EMA smooths noisy input but introduces lag. The raw critical-temperature override prevents that lag from delaying a critical fan command.
- **Hysteresis vs. precision:** different rising and falling thresholds reduce frequent switching around a boundary.
- **Immediate fail-safe vs. availability:** the example commands HIGH on any unusable sensor sample; a real product needs a hazard analysis to choose the correct safe action.
- **Deterministic scenarios vs. realism:** fixed scenario profiles make debugging and tests repeatable, but they do not represent all physical operating conditions.

## Limitations

- No physical sensor, fan, GPIO, PWM, RTOS, or microcontroller is used.
- The thermal model is intentionally simplified and unvalidated.
- The RPM output is calculated from a lookup table, not a tachometer.
- `--realtime` paces a software loop but does not provide hard real-time guarantees.
