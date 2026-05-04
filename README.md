# STM32 Mini PLC

Custom miniature industrial PLC built from scratch on STM32F103C8T6.
Features ladder logic VM, visual editor, EEPROM persistence, OLED display,
RS485 Modbus RTU, and ESP32 web dashboard.

> B.Tech Major Project — May 2026

## Demo Videos

| Feature | Status |
|---|---|
| Visual ladder editor + download | recorded |
| Live monitoring with green highlights | recorded |
| OLED standalone display | recorded |
| ESP32 web dashboard | recorded |
| RS485 Modbus communication | recorded |

## Features

- Custom 12-opcode ladder logic VM with 100Hz scan cycle
- Visual ladder editor with parallel branches and LDEQ state machines
- Live monitoring panel — elements highlight when active
- EEPROM persistent storage with 4 program slots
- Auto-boot — runs last saved program on startup
- Standalone OLED display (1.3" SH1106)
- RS485 Modbus RTU slave (FC01/FC03/FC05)
- ESP32 web dashboard for remote monitoring
- 14-program demo library

## Hardware

| Component | Purpose |
|---|---|
| STM32F103C8T6 | Main PLC controller |
| PC817 ×8 | Isolated industrial inputs |
| 2N2222 + relay | Industrial output Q0 |
| AT24C64 | Program EEPROM |
| SH1106 OLED | Status display |
| MAX485 | RS485 transceiver |
| ESP32 | WiFi bridge |

## Tech Stack

STM32 HAL · Custom ladder VM · Python (CustomTkinter) · ESP32 Arduino · Modbus RTU

## What I Learned

- Designing a custom bytecode VM for embedded targets
- Real-time scan cycles with hardware timers
- I2C bus management with multiple devices
- Modbus RTU with CRC-16 and half-duplex RS485
- Cross-platform GUI compiler in Python
- Serial communication threading and locking