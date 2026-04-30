#include "eeprom.h"
#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

#define EEPROM_TIMEOUT_MS 100

// ============================================================
// Low-level I2C operations (using HAL_I2C_Mem_* for reliability)
// ============================================================

bool eeprom_init(void) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_I2C_ADDR, 3, EEPROM_TIMEOUT_MS) != HAL_OK) {
        return false;
    }
    uint8_t dummy;
    return eeprom_read_byte(0, &dummy);
}

bool eeprom_read_byte(uint16_t addr, uint8_t *value) {
    return HAL_I2C_Mem_Read(&hi2c1, EEPROM_I2C_ADDR, addr,
                            I2C_MEMADD_SIZE_16BIT, value, 1,
                            EEPROM_TIMEOUT_MS) == HAL_OK;
}

bool eeprom_write_byte(uint16_t addr, uint8_t value) {
    if (HAL_I2C_Mem_Write(&hi2c1, EEPROM_I2C_ADDR, addr,
                          I2C_MEMADD_SIZE_16BIT, &value, 1,
                          EEPROM_TIMEOUT_MS) != HAL_OK) {
        return false;
    }
    HAL_Delay(10);  // wait for internal write cycle to complete
    return true;
}

bool eeprom_read_block(uint16_t addr, uint8_t *data, uint16_t len) {
    if (len == 0) return true;
    return HAL_I2C_Mem_Read(&hi2c1, EEPROM_I2C_ADDR, addr,
                            I2C_MEMADD_SIZE_16BIT, data, len,
                            EEPROM_TIMEOUT_MS + len) == HAL_OK;
}

bool eeprom_write_block(uint16_t addr, const uint8_t *data, uint16_t len) {
    // Write byte-by-byte (simple and safe across page boundaries)
    for (uint16_t i = 0; i < len; i++) {
        if (!eeprom_write_byte(addr + i, data[i])) {
            return false;
        }
    }
    return true;
}

bool eeprom_test(void) {
    if (!eeprom_write_byte(0x0000, 0xA5)) return false;
    if (!eeprom_write_byte(0x0001, 0x5A)) return false;

    uint8_t v1 = 0, v2 = 0;
    if (!eeprom_read_byte(0x0000, &v1)) return false;
    if (!eeprom_read_byte(0x0001, &v2)) return false;

    return (v1 == 0xA5 && v2 == 0x5A);
}

// ============================================================
// Slot management
// ============================================================

static uint16_t slot_address(uint8_t slot) {
    return (uint16_t)slot * EEPROM_SLOT_SIZE;
}

// Read the entire 4-byte header in ONE I2C transaction
static bool read_slot_header(uint8_t slot, uint8_t header[4]) {
    if (slot >= EEPROM_SLOT_COUNT) return false;
    return eeprom_read_block(slot_address(slot), header, 4);
}

bool eeprom_save_slot(uint8_t slot, const uint8_t *data, uint16_t len) {
    if (slot >= EEPROM_SLOT_COUNT) return false;
    if (len > EEPROM_SLOT_DATA_MAX) return false;

    uint16_t base = slot_address(slot);

    uint8_t header[4];
    header[0] = EEPROM_MAGIC_1;
    header[1] = EEPROM_MAGIC_2;
    header[2] = len & 0xFF;
    header[3] = (len >> 8) & 0xFF;

    // Write data first, then header (crash-safe)
    if (!eeprom_write_block(base + 4, data, len)) return false;
    if (!eeprom_write_block(base, header, 4)) return false;

    return true;
}

bool eeprom_slot_is_valid(uint8_t slot) {
    uint8_t header[4];
    if (!read_slot_header(slot, header)) return false;
    return (header[0] == EEPROM_MAGIC_1 && header[1] == EEPROM_MAGIC_2);
}

uint16_t eeprom_slot_length(uint8_t slot) {
    uint8_t header[4];
    if (!read_slot_header(slot, header)) return 0;
    if (header[0] != EEPROM_MAGIC_1 || header[1] != EEPROM_MAGIC_2) return 0;
    return ((uint16_t)header[3] << 8) | header[2];
}

bool eeprom_load_slot(uint8_t slot, uint8_t *data, uint16_t *len_out, uint16_t max_len) {
    if (!eeprom_slot_is_valid(slot)) return false;

    uint16_t len = eeprom_slot_length(slot);
    if (len == 0 || len > max_len) return false;

    if (!eeprom_read_block(slot_address(slot) + 4, data, len)) return false;
    *len_out = len;
    return true;
}
