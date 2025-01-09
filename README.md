# Sensor Data Processing System

## Overview
This repository implements a system for reading, processing, and transmitting sensor data. It is designed to work with multiple sensors, communicate over an RF interface, and display data on an LCD.

---

## Features
- **Sensor Integration**:
  - Supports various sensors, including SHTC3, BMP388, and others.
  - Reads data such as temperature, humidity, CO2 levels, and light intensity.
- **RF Communication**:
  - Transmits sensor data using a custom protocol.
  - Includes test cases for validating communication.
- **Data Display**:
  - Displays sensor data on an LCD.
  - Formats data for clear readability (e.g., float values with decimal precision).
- **Custom Scheduler**:
  - Implements a real-time task scheduler for managing sensor data collection and transmission tasks.

---

## Repository Structure

```
.
├── lib/                  # Libraries for LCD, I2C, and utility functions
├── communication/        # RF adapter and sensor data protocols
├── progs/                # Test cases and task implementations
├── os_core.c             # Core OS functionality
├── os_scheduler.c        # Task scheduler and ISR
├── sensorData.h          # Definitions for sensor types and parameters
├── README.md             # Project documentation
```

---

## How It Works
1. **Sensor Initialization**:
   - Initializes sensors like SHTC3 using I2C.
   - Configures supported sensors to collect data.
2. **Data Processing**:
   - Simulates or reads actual sensor data.
   - Formats data for transmission and display.
3. **RF Transmission**:
   - Encodes data packets with sensor type, parameter type, and value.
   - Sends data to a receiving device or broadcast address.
4. **Display**:
   - Outputs sensor data on an LCD.
   - Handles formatted output for various data types.

---

## Configuration
### RF Adapter
- The RF adapter communicates with other devices using commands defined in `rfAdapter.h`.
- Modify `PARTNER_ADDRESS` in the test cases to set the target address for transmission.

### Scheduler
- The scheduler is implemented in `os_scheduler.c` and supports multiple task priorities.
- Tasks are registered and started using `os_registerProgram` and `os_exec`.

---

## Supported Sensors
| Sensor         | Parameters Supported               |
|----------------|------------------------------------|
| SHTC3          | Temperature (°C), Humidity (%)    |
| BMP388         | Temperature (°C), Pressure (Pa)   |
| ALS-PT19       | Light Intensity (%)               |
| AM2320         | Temperature (°C), Humidity (%)    |

---

## Testing
- The `progs/` folder includes test cases for validating sensor data collection and transmission.
- Test programs simulate sensor values and check LCD and RF communication outputs.

---

## Getting Started
### Prerequisites
- Atmel AVR Toolchain
- AVR microcontroller (e.g., ATmega2560)
- Connected sensors (e.g., SHTC3)
- LCD for data display

### Building and Uploading
1. Clone the repository:
   ```bash
   git clone <repository-url>
   ```
2. Build the project:
   ```bash
   make all
   ```
3. Upload the firmware to your microcontroller:
   ```bash
   make flash
   ```

---

## References
- [SHTC3 Datasheet](https://www.sensirion.com/file/datasheet.pdf)
- [I2C Communication Protocol](https://www.nxp.com/documents/user_manual/UM10204.pdf)

---

## License
This project is licensed under the MIT License. See `LICENSE` for details.
