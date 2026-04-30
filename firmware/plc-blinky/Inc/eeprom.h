#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include <stdint.h>
#include <stdbool.h>

// AT24C64 — 8 KB I2C EEPROM at address 0x50
#define EEPROM_I2C_ADDR  (0x50 << 1)   // HAL expects 8-bit address
#define EEPROM_TOTAL_SIZE 8192          // bytes
#define EEPROM_PAGE_SIZE  32            // for page writes

bool eeprom_init(void);
bool eeprom_test(void);  // simple test: write byte 0xA5 at addr 0, read back

bool eeprom_write_byte(uint16_t addr, uint8_t value);
bool eeprom_read_byte(uint16_t addr, uint8_t *value);

bool eeprom_write_block(uint16_t addr, const uint8_t *data, uint16_t len);
bool eeprom_read_block(uint16_t addr, uint8_t *data, uint16_t len);

// Slot management
#define EEPROM_SLOT_COUNT 4
#define EEPROM_SLOT_SIZE  2048
#define EEPROM_SLOT_DATA_MAX (EEPROM_SLOT_SIZE - 4)  // minus 4-byte header
#define EEPROM_MAGIC_1 0xAA
#define EEPROM_MAGIC_2 0x55

bool eeprom_save_slot(uint8_t slot, const uint8_t *data, uint16_t len);
bool eeprom_load_slot(uint8_t slot, uint8_t *data, uint16_t *len_out, uint16_t max_len);
bool eeprom_slot_is_valid(uint8_t slot);
uint16_t eeprom_slot_length(uint8_t slot);  // returns 0 if invalid
#endif /* INC_EEPROM_H_ */
