/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Modbus RTU Master — STM32 #2
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* USER CODE BEGIN PM */
/* USER CODE END PM */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
int __io_putchar(int ch);
static uint16_t crc16(const uint8_t *data, uint16_t len);
static uint8_t modbus_transaction(uint8_t *req, uint8_t req_len,
                                   uint8_t *resp, uint8_t max_resp);
static void read_coils(void);
static void read_scan_count(void);
static void write_coil(uint8_t coil_addr, bool on);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

// Redirect printf to USART1 (console → PC via USB-TTL)
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
    return ch;
}

// CRC-16 Modbus polynomial 0xA001
static uint16_t crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

#define SLAVE_ADDR           0x01
#define FC_READ_COILS        0x01
#define FC_READ_HOLDING_REGS 0x03
#define FC_WRITE_SINGLE_COIL 0x05

// Send Modbus request on USART2, receive response
// Returns response length or 0 on timeout/error
static uint8_t modbus_transaction(uint8_t *req, uint8_t req_len,
                                   uint8_t *resp, uint8_t max_resp) {
    // Append CRC to request
    uint16_t crc = crc16(req, req_len);
    req[req_len]     = crc & 0xFF;
    req[req_len + 1] = (crc >> 8) & 0xFF;

    // Flush RX buffer
    __HAL_UART_FLUSH_DRREGISTER(&huart2);

    // Send request
    HAL_UART_Transmit(&huart2, req, req_len + 2, 100);

    // Receive response with timeout
    uint8_t resp_len = 0;
    uint32_t deadline = HAL_GetTick() + 300;

    while (HAL_GetTick() < deadline && resp_len < max_resp) {
        if (HAL_UART_Receive(&huart2, &resp[resp_len], 1, 20) == HAL_OK) {
            resp_len++;
            deadline = HAL_GetTick() + 50;
        }
    }

    if (resp_len < 4) return 0;

    // Verify CRC
    uint16_t rx_crc   = ((uint16_t)resp[resp_len-1] << 8) | resp[resp_len-2];
    uint16_t calc_crc = crc16(resp, resp_len - 2);
    if (rx_crc != calc_crc) {
        printf("[CRC ERROR] rx=%04X calc=%04X len=%d\r\n",
               rx_crc, calc_crc, resp_len);
        return 0;
    }

    return resp_len;
}

// FC01 — Read 8 coils: coils 0-3 = Q0-Q3 outputs, coils 4-7 = I0-I3 inputs
static void read_coils(void) {
    uint8_t req[8];
    uint8_t resp[16];

    req[0] = SLAVE_ADDR;
    req[1] = FC_READ_COILS;
    req[2] = 0x00;
    req[3] = 0x00;  // start address: coil 0
    req[4] = 0x00;
    req[5] = 0x08;  // quantity: 8 coils

    uint8_t len = modbus_transaction(req, 6, resp, sizeof(resp));
    if (len == 0) {
        printf("[FC01] No response from slave\r\n");
        return;
    }
    if (resp[1] & 0x80) {
        printf("[FC01] Exception code: %02X\r\n", resp[2]);
        return;
    }

    uint8_t coils = resp[3];
    printf("[FC01] Q0=%d Q1=%d Q2=%d Q3=%d | I0=%d I1=%d I2=%d I3=%d\r\n",
           (coils >> 0) & 1, (coils >> 1) & 1,
           (coils >> 2) & 1, (coils >> 3) & 1,
           (coils >> 4) & 1, (coils >> 5) & 1,
           (coils >> 6) & 1, (coils >> 7) & 1);
}

// FC03 — Read holding registers: reg0=scan count low, reg1=scan count high
static void read_scan_count(void) {
    uint8_t req[8];
    uint8_t resp[16];

    req[0] = SLAVE_ADDR;
    req[1] = FC_READ_HOLDING_REGS;
    req[2] = 0x00;
    req[3] = 0x00;  // register 0
    req[4] = 0x00;
    req[5] = 0x02;  // read 2 registers

    uint8_t len = modbus_transaction(req, 6, resp, sizeof(resp));
    if (len == 0) {
        printf("[FC03] No response from slave\r\n");
        return;
    }
    if (resp[1] & 0x80) {
        printf("[FC03] Exception code: %02X\r\n", resp[2]);
        return;
    }

    uint16_t reg0 = ((uint16_t)resp[3] << 8) | resp[4];
    uint16_t reg1 = ((uint16_t)resp[5] << 8) | resp[6];
    uint32_t scan = ((uint32_t)reg1 << 16) | reg0;
    printf("[FC03] PLC Scan count: %lu\r\n", scan);
}

// FC05 — Write single coil ON or OFF (coil 0-3 = Q0-Q3)
static void write_coil(uint8_t coil_addr, bool on) {
    uint8_t req[8];
    uint8_t resp[16];

    req[0] = SLAVE_ADDR;
    req[1] = FC_WRITE_SINGLE_COIL;
    req[2] = 0x00;
    req[3] = coil_addr;
    req[4] = on ? 0xFF : 0x00;
    req[5] = 0x00;

    uint8_t len = modbus_transaction(req, 6, resp, sizeof(resp));
    if (len == 0) {
        printf("[FC05] No response from slave\r\n");
        return;
    }
    if (resp[1] & 0x80) {
        printf("[FC05] Exception code: %02X\r\n", resp[2]);
        return;
    }

    printf("[FC05] Coil %d set %s OK\r\n", coil_addr, on ? "ON" : "OFF");
}

/* USER CODE END 0 */

int main(void) {

    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    printf("\r\n==============================\r\n");
    printf("  Modbus RTU Master\r\n");
    printf("  STM32 Mini PLC Demo\r\n");
    printf("==============================\r\n");
    printf("Slave addr: 0x01\r\n");
    printf("Baud: 9600 on USART2\r\n");
    printf("Console: 115200 on USART1\r\n");
    printf("PB0 = Force Q0 ON\r\n");
    printf("PB1 = Force Q0 OFF\r\n\r\n");

    uint32_t last_coil_tick = 0;
    uint32_t last_scan_tick = 0;
    bool btn0_prev = true;
    bool btn1_prev = true;
    /* USER CODE END 2 */

    while (1) {
        /* USER CODE BEGIN 3 */
        uint32_t now = HAL_GetTick();

        // Read coils every 1 second
        if (now - last_coil_tick >= 1000) {
            last_coil_tick = now;
            read_coils();
        }

        // Read scan count every 3 seconds
        if (now - last_scan_tick >= 3000) {
            last_scan_tick = now;
            read_scan_count();
        }

        // PB0 pressed (active LOW) — force Q0 ON via Modbus
        bool btn0 = (HAL_GPIO_ReadPin(BTN_ON_GPIO_Port, BTN_ON_Pin) == GPIO_PIN_RESET);
        if (btn0 && !btn0_prev) {
            write_coil(0, true);
        }
        btn0_prev = btn0;

        // PB1 pressed (active LOW) — force Q0 OFF via Modbus
        bool btn1 = (HAL_GPIO_ReadPin(BTN_OFF_GPIO_Port, BTN_OFF_Pin) == GPIO_PIN_RESET);
        if (btn1 && !btn1_prev) {
            write_coil(0, false);
        }
        btn1_prev = btn1;

        HAL_Delay(10);
        /* USER CODE END 3 */
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // PB0 = BTN_ON, PB1 = BTN_OFF — input with pull-up
    GPIO_InitStruct.Pin = BTN_ON_Pin | BTN_OFF_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
