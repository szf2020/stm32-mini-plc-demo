#include "modbus.h"
#include "io_image.h"
#include "scan_engine.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

// ============================================================
// CRC-16 (Modbus polynomial 0xA001)
// ============================================================
static uint16_t crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// ============================================================
// RS485 direction control
// ============================================================
static void rs485_tx_mode(void) {
    HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_SET);
}

static void rs485_rx_mode(void) {
    HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_RESET);
}

// ============================================================
// Frame reception
// ============================================================
static uint8_t  rx_buf[MODBUS_RX_BUFFER_SIZE];
static uint8_t  rx_byte_raw;
static volatile uint8_t  rx_len = 0;
static volatile uint32_t last_rx_tick = 0;

void modbus_init(void) {
    rs485_rx_mode();
    rx_len = 0;

    HAL_StatusTypeDef st = HAL_UART_Receive_IT(&huart2, &rx_byte_raw, 1);
    printf("MB: init RX_IT=%d, RxState=%d\r\n", st, huart2.RxState);

    // SELF-TEST: send a byte from UART2 TX, see if it loops back to RX
    HAL_Delay(100);
    rs485_tx_mode();    // Doesn't matter, MAX485 is disconnected
    uint8_t test = 0xAA;
    HAL_UART_Transmit(&huart2, &test, 1, 100);
    HAL_Delay(50);
    rs485_rx_mode();
    printf("MB: self-test sent 0xAA, rx_len=%d\r\n", rx_len);
}

void modbus_rx_byte(uint8_t byte) {
    if (rx_len < MODBUS_RX_BUFFER_SIZE) {
        rx_buf[rx_len++] = byte;
        last_rx_tick = HAL_GetTick();
    }
    HAL_UART_Receive_IT(&huart2, &rx_byte_raw, 1);
}

void modbus_rx_byte_from_isr(void) {
    modbus_rx_byte(rx_byte_raw);
}

// ============================================================
// Send response
// ============================================================
static void send_response(uint8_t *buf, uint8_t len) {
    uint16_t crc = crc16(buf, len);
    buf[len]     = crc & 0xFF;
    buf[len + 1] = (crc >> 8) & 0xFF;

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    rs485_tx_mode();
    HAL_Delay(2);

    HAL_UART_Transmit(&huart2, buf, len + 2, 100);

    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET);
    HAL_Delay(3);

    rs485_rx_mode();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    printf("MB TX: ");
    for (int i = 0; i < len + 2; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");
}

// ============================================================
// Exception response
// ============================================================
static void send_exception(uint8_t func_code, uint8_t exception_code) {
    uint8_t buf[5];
    buf[0] = MODBUS_SLAVE_ADDRESS;
    buf[1] = func_code | 0x80;
    buf[2] = exception_code;
    send_response(buf, 3);
}

// ============================================================
// FC01 — Read Coils
// ============================================================
static void handle_read_coils(uint8_t *frame) {
    uint16_t start_addr = ((uint16_t)frame[2] << 8) | frame[3];
    uint16_t quantity   = ((uint16_t)frame[4] << 8) | frame[5];

    if (quantity == 0 || quantity > 8 || (start_addr + quantity) > 8) {
        send_exception(FC_READ_COILS, 0x02);
        return;
    }

    uint8_t buf[6];
    buf[0] = MODBUS_SLAVE_ADDRESS;
    buf[1] = FC_READ_COILS;
    buf[2] = 1;

    uint8_t coil_byte = 0;
    for (uint8_t i = 0; i < quantity; i++) {
        uint8_t addr = start_addr + i;
        uint8_t state = 0;
        if (addr < 4) {
            state = g_io.digital_out[addr] ? 1 : 0;
        } else if (addr < 8) {
            state = g_io.digital_in[addr - 4] ? 1 : 0;
        }
        if (state) coil_byte |= (1 << i);
    }
    buf[3] = coil_byte;

    send_response(buf, 4);
}

// ============================================================
// FC05 — Write Single Coil
// ============================================================
static void handle_write_single_coil(uint8_t *frame) {
    uint16_t coil_addr  = ((uint16_t)frame[2] << 8) | frame[3];
    uint16_t coil_value = ((uint16_t)frame[4] << 8) | frame[5];

    if (coil_addr > 3) {
        send_exception(FC_WRITE_SINGLE_COIL, 0x02);
        return;
    }
    if (coil_value != COIL_ON && coil_value != COIL_OFF) {
        send_exception(FC_WRITE_SINGLE_COIL, 0x03);
        return;
    }

    g_io.digital_out[coil_addr] = (coil_value == COIL_ON) ? 1 : 0;

    uint8_t buf[8];
    buf[0] = MODBUS_SLAVE_ADDRESS;
    buf[1] = FC_WRITE_SINGLE_COIL;
    buf[2] = frame[2];
    buf[3] = frame[3];
    buf[4] = frame[4];
    buf[5] = frame[5];
    send_response(buf, 6);
}

// ============================================================
// FC03 — Read Holding Registers
// ============================================================
static void handle_read_holding_regs(uint8_t *frame) {
    uint16_t start_addr = ((uint16_t)frame[2] << 8) | frame[3];
    uint16_t quantity   = ((uint16_t)frame[4] << 8) | frame[5];

    if (quantity == 0 || quantity > 2 || start_addr > 1) {
        send_exception(FC_READ_HOLDING_REGS, 0x02);
        return;
    }

    uint8_t buf[10];
    buf[0] = MODBUS_SLAVE_ADDRESS;
    buf[1] = FC_READ_HOLDING_REGS;
    buf[2] = (uint8_t)(quantity * 2);

    uint32_t scan = scan_get_count();
    uint16_t reg0 = (uint16_t)(scan & 0xFFFF);
    uint16_t reg1 = (uint16_t)((scan >> 16) & 0xFFFF);

    uint8_t idx = 3;
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t reg = (start_addr + i == 0) ? reg0 : reg1;
        buf[idx++] = (reg >> 8) & 0xFF;
        buf[idx++] = reg & 0xFF;
    }

    send_response(buf, idx);
}

// ============================================================
// Main poll — WITH DEBUG PRINTS
// ============================================================
void modbus_poll(void) {
    static uint32_t last_debug = 0;

    // Heartbeat — prints rx_len every 2 seconds
    if (HAL_GetTick() - last_debug > 2000) {
        last_debug = HAL_GetTick();
        printf("MB poll: rx_len=%d\r\n", rx_len);
    }

    if (rx_len == 0) return;
    if ((HAL_GetTick() - last_rx_tick) < MODBUS_FRAME_TIMEOUT_MS) return;

    uint8_t len = rx_len;
    rx_len = 0;

    printf("MB RX: len=%d ", len);
    for (int i = 0; i < len; i++) {
        printf("%02X ", rx_buf[i]);
    }
    printf("\r\n");

    if (len < 4) {
        printf("MB ERR: too short\r\n");
        return;
    }

    if (rx_buf[0] != MODBUS_SLAVE_ADDRESS) {
        printf("MB ERR: wrong addr %d\r\n", rx_buf[0]);
        return;
    }

    uint16_t received_crc = ((uint16_t)rx_buf[len-1] << 8) | rx_buf[len-2];
    uint16_t calculated_crc = crc16(rx_buf, len - 2);

    if (received_crc != calculated_crc) {
        printf("MB ERR: CRC fail\r\n");
        return;
    }

    printf("MB OK: FC=%02X\r\n", rx_buf[1]);

    switch (rx_buf[1]) {
        case FC_READ_COILS:
            handle_read_coils(rx_buf);
            break;
        case FC_WRITE_SINGLE_COIL:
            handle_write_single_coil(rx_buf);
            break;
        case FC_READ_HOLDING_REGS:
            handle_read_holding_regs(rx_buf);
            break;
        default:
            send_exception(rx_buf[1], 0x01);
            break;
    }
}
