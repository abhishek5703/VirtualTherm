# VirtualTherm: interview quick guide

## 30-second explanation

"VirtualTherm is a C++17 software simulation of an embedded cooling controller. It models a temperature-sensitive system, reads a virtual sensor, filters the signal, and adjusts a simulated fan using a state machine with hysteresis. I added a critical-temperature latch, sensor-disconnect recovery checks, several repeatable operating scenarios, and CSV diagnostics. I built it without hardware to practise embedded control logic and fault handling; the results are simulation data, not measurements from a physical device."

## Likely interview questions

### Why did you use C++?
C++ gives the project small classes with clear responsibilities and lets the control logic stay portable. The same interfaces could later be connected to target-specific sensor and fan drivers, although this prototype itself is not firmware.

### Why add a filter?
A virtual sensor can produce small fluctuations. An exponential moving average smooths them using `filtered = alpha * current + (1 - alpha) * previous`. A higher alpha responds more quickly but smooths less; a lower alpha smooths more but adds lag.

### Why do you need hysteresis if you already have a filter?
Filtering reduces measurement noise; hysteresis changes the switching thresholds depending on the current fan mode. They address related but different problems. Together they reduce unnecessary fan-mode changes.

### Why is critical temperature latched?
A brief dip below the trip threshold should not immediately clear a critical condition. This demo keeps the controller in CRITICAL until both raw and filtered readings cool below a separate reset threshold.

### What happens if the sensor fails?
The controller immediately commands the simulated fan to HIGH and reports SENSOR_FAULT. After the sensor readings become valid again, it requires three consecutive good samples before resuming temperature-based control. A real system must determine the safe output from a hazard analysis.

### What are the scenarios?
- `normal`: a changing workload with typical control transitions.
- `overheat`: sustained heavy load to exercise the critical latch.
- `sensor-fault`: a repeatable ten-second disconnect followed by recovery.
- `workload-spike`: a sharp load increase followed by cooling recovery.
- `noisy-sensor`: deterministic measurement noise to exercise the filter.

### How did you test it?
The automated test program covers fan thresholds, hysteresis, critical latching, fault recovery, filter reset behavior, scenario parsing, thermal model behavior, and end-to-end overheat and sensor-fault scenarios. I can also inspect the CSV output to understand every sample and state transition.

### What would you do next with hardware?
Replace the virtual sensor and thermal model with a real sensor driver and safe low-voltage fan driver, then verify driver behavior, timing, fault handling, and measured fan response on the target. Only after hardware testing would it be appropriate to make claims about real motor or thermal behavior.

## Resume bullet

Developed **VirtualTherm**, a C++17 software simulation of an embedded cooling controller featuring EMA sensor filtering, hysteresis-based fan control, critical-temperature latching, sensor-fault recovery checks, repeatable load scenarios, CSV diagnostics, and automated tests.

Use the bullet only after reviewing, building, and testing the project yourself. Be explicit that it is software-only simulation and has not been verified on physical hardware.
