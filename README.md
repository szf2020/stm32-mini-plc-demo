# STM32 Mini Industrial PLC

A microcontroller-based programmable logic controller built on STM32F103 (Blue Pill), 
featuring 8 digital inputs, 4 relay outputs, 4 transistor outputs, 2 analog inputs, 
and a custom ladder logic interpreter.

**Status:** In development (B.Tech major project)

## Specifications
- 24V industrial I/O
- 8x optically-isolated digital inputs
- 4x relay outputs (5V SPDT)
- 4x transistor outputs
- 2x analog inputs (0–3.3V, 12-bit)
- External I2C EEPROM for program storage
- RS485 + UART interfaces
- Deterministic 10 ms scan cycle

## Repository layout
- `firmware/` — STM32 C code
- `hardware/` — schematics and PCB
- `tools/` — PC-side ladder editor
- `docs/` — design documents
- `demo-programs/` — example ladder programs
