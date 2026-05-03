#ifndef INC_MODBUS_H_
#define INC_MODBUS_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// Modbus configuration
#define MODBUS_SLAVE_ADDRESS    0x01
#define MODBUS_RX_BUFFER_SIZE   64
#define MODBUS_FRAME_TIMEOUT_MS 5

// Function codes
#define FC_READ_COILS           0x01
#define FC_READ_HOLDING_REGS    0x03
#define FC_WRITE_SINGLE_COIL    0x05

// Coil values
#define COIL_ON                 0xFF00
#define COIL_OFF                0x0000

// Public API
void modbus_init(void);
void modbus_poll(void);
void modbus_rx_byte(uint8_t byte);
void modbus_rx_byte_from_isr(void);

#endif /* INC_MODBUS_H_ */
