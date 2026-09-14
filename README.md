# Feedog

Automatic dog feeding and water-management system based on the RP2040.

Feedog is an embedded systems project developed for Digital Electronics III. The system combines sensors, actuators, an OLED user interface, an RTC, and a navigation panel to automate food dispensing and water management.

**Project type:** Embedded Systems / Digital Electronics
**Microcontroller:** Raspberry Pi RP2040
**Language:** C
**Development environment:** Raspberry Pi Pico SDK

## Overview

Feedog was designed as an automated system capable of managing a dog's food and water supply according to configured schedules and sensor feedback.

The system includes:

* Automatic food dispensing using a servo-driven gate.
* Automatic water refilling using a pump.
* Water-level monitoring.
* Ultrasonic measurement for food-bowl monitoring.
* Real-time clock for date and time management.
* OLED display with a configuration and monitoring interface.
* Navigation panel with multiple push buttons.
* Manual and automatic operating modes.

PLACEHOLDER: IMAGES/FEEDOG_SYSTEM_OVERVIEW.PNG

## System Architecture

The RP2040 acts as the central controller and coordinates the sensors, user interface, RTC, and actuators.

PLACEHOLDER: IMAGES/SYSTEM_ARCHITECTURE.PNG

Main system elements:

| Component          | Function                              |
| ------------------ | ------------------------------------- |
| RP2040             | Main microcontroller                  |
| OLED display       | User interface and system information |
| Push buttons       | Menu navigation and configuration     |
| DS3231 RTC         | Date and time management              |
| Ultrasonic sensor  | Food-bowl distance measurement        |
| Water-level sensor | Water-level monitoring                |
| Servo motor        | Food gate actuation                   |
| Water pump         | Automatic water filling               |

## Hardware

The project integrates the following hardware components:

* Raspberry Pi RP2040
* OLED display
* DS3231 real-time clock
* Servo motor
* Water pump
* Ultrasonic sensor
* Water-level sensor
* Push-button navigation panel
* Transistor-based pump switching circuit

PLACEHOLDER: HARDWARE/OLED.PNG

PLACEHOLDER: HARDWARE/RTC.PNG

PLACEHOLDER: HARDWARE/SERVO.PNG

PLACEHOLDER: HARDWARE/ULTRASONICSENSOR.PNG

PLACEHOLDER: HARDWARE/LEVELSENSOR.PNG

PLACEHOLDER: HARDWARE/PUMP.PNG

## Firmware

The firmware is written in C using the Raspberry Pi Pico SDK.

The application is organized around different operating states, including:

* Startup
* Welcome screen
* Date configuration
* Time configuration
* Mode selection
* Manual mode
* Automatic mode
* Monitoring
* Food schedule configuration
* Water schedule configuration

The main application handles the interaction between the user interface, RTC, sensors, and actuators.

PLACEHOLDER: FLOWCHARTS/SYSTEM.PNG

### Sensor and Actuator Control

The firmware includes dedicated modules for several hardware functions:

* `ultrasonic.c / ultrasonic.h` — ultrasonic distance measurement.
* `servo.c / servo.h` — servo control.
* `BitBang_I2C.c / BitBang_I2C.h` — software I2C communication.
* `ss_oled.c / ss_oled.h` — OLED display control.

PLACEHOLDER: FLOWCHARTS/ULTRASONICSENSOR.PNG

PLACEHOLDER: FLOWCHARTS/SERVO.PNG

PLACEHOLDER: FLOWCHARTS/I2C.PNG

PLACEHOLDER: FLOWCHARTS/OLED.PNG

## Automatic Operation

In automatic mode, the system uses the RTC to compare the current time against the configured feeding and watering schedules.

When a scheduled event is reached:

1. The corresponding function is triggered.
2. The servo operates the food gate or the pump is activated.
3. Sensor information is used to monitor the corresponding bowl.
4. The system returns to its normal monitoring state.

PLACEHOLDER: FLOWCHARTS/MAIN.PNG

## Manual Operation

Manual mode allows the user to directly activate the food dispensing and water filling mechanisms through the navigation interface.

This mode was included for system testing and direct control of the actuators.

PLACEHOLDER: EXTRAS/MANUAL_MODE.PNG

## Project Resources

The repository includes the original project documentation and design resources:

```text
Feedog/
├── flowcharts/
├── hardware/
├── src/
├── .gitignore
├── LICENSE
└── README.md
```

The `flowcharts/` directory contains the original system and module flowcharts.

The `hardware/` directory contains hardware-related diagrams and component representations.

The `src/` directory contains the project firmware source code.

## Project Status

This repository preserves the original firmware and documentation from the academic development of Feedog.

The code is intentionally kept close to its original state rather than being refactored into a modern production-oriented architecture. As a historical academic project, it may contain implementation limitations or imperfections that reflect the original development process.

The project is presented as a record of an embedded systems and electronics development project rather than as production-ready firmware.

## Third-Party Components

This project includes third-party software components from **BitBank Software**, specifically:

* `BitBang_I2C`
* `ss_oled`

These components were written by Larry Bank and are distributed under the **GNU General Public License v3.0 (GPL-3.0)**.

The original copyright and license notices included with these source files have been preserved.

The Feedog project is distributed under the GNU GPL v3.0 in accordance with the licensing requirements applicable to the incorporated GPL-covered components.

## License

This project is distributed under the **GNU General Public License v3.0**.

See the [`LICENSE`](LICENSE) file for the complete license text.

Third-party components retain their original copyright and licensing notices.

## Academic Context

Feedog was developed as part of the Digital Electronics III coursework, with the objective of integrating digital electronics, embedded programming, sensors, actuators, and user interaction into a complete working system.

PLACEHOLDER: EXTRAS/FINAL_PROJECT_PHOTO.PNG
