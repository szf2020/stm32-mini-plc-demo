#include "uart_console.h"
#include "main.h"
#include "io_image.h"
#include "scan_engine.h"
#include "ladder_vm.h"
#include "ladder_programs.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

#define RX_BUFFER_SIZE 64

static char rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_index = 0;
static volatile uint8_t  cmd_ready = 0;
static uint8_t rx_byte;

// Currently active program — defaults to motor
static const uint8_t *active_program = motor_start_stop_program;
static uint16_t active_program_len = MOTOR_PROGRAM_LEN;

// printf redirect — sends each character via UART
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

static void print_banner(void) {
    printf("\r\n");
    printf("====================================\r\n");
    printf("  STM32 Mini PLC - UART Console\r\n");
    printf("====================================\r\n");
    printf("Type 'help' for commands.\r\n");
    printf("> ");
}

void uart_console_init(void) {
    rx_index = 0;
    cmd_ready = 0;

    // Start single-byte receive in interrupt mode
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    print_banner();
}

// Called inside the UART RX interrupt (via HAL_UART_RxCpltCallback)
void uart_console_handle_rx(uint8_t byte) {
    if (cmd_ready) return;

    // Echo the typed character back to terminal (so user can see typing)
    HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);

    if (byte == '\r' || byte == '\n') {
        // Send line break for cleaner display
        uint8_t lf = '\n';
        HAL_UART_Transmit(&huart1, &lf, 1, HAL_MAX_DELAY);
        if (rx_index > 0) {
            rx_buffer[rx_index] = '\0';
            cmd_ready = 1;
        } else {
            // Empty line, just print prompt again
            printf("> ");
        }
    } else if (byte == 0x08 || byte == 0x7F) {
        // Backspace
        if (rx_index > 0) {
            rx_index--;
        }
    } else if (rx_index < RX_BUFFER_SIZE - 1) {
        rx_buffer[rx_index++] = byte;
    }
}

// Wrapper called from HAL_UART_RxCpltCallback in main.c
void uart_console_on_rx(void) {
    uart_console_handle_rx(rx_byte);
    // Re-arm the receive
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

static void process_command(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        printf("\r\nCommands:\r\n");
        printf("  help            - this message\r\n");
        printf("  status          - show PLC state\r\n");
        printf("  load motor      - run motor start/stop\r\n");
        printf("  load timer      - run 2-second timer demo\r\n");
        printf("  load echo       - run echo (input -> output)\r\n");
        printf("  load not        - run NOT gate\r\n");
        printf("  load counter    - run 5-press counter\r\n");
        printf("  reset           - reset all I/O\r\n");
    }
    else if (strcmp(cmd, "status") == 0) {
        printf("\r\nPLC Status:\r\n");
        printf("  Inputs:  I0=%d I1=%d\r\n",
               g_io.digital_in[0], g_io.digital_in[1]);
        printf("  Outputs: Q0=%d\r\n", g_io.digital_out[0]);
        printf("  Scan count: %lu\r\n", scan_get_count());
    }
    else if (strcmp(cmd, "load motor") == 0) {
        active_program = motor_start_stop_program;
        active_program_len = MOTOR_PROGRAM_LEN;
        printf("\r\nLoaded: motor start/stop\r\n");
    }
    else if (strcmp(cmd, "load timer") == 0) {
        active_program = timer_demo_program;
        active_program_len = TIMER_DEMO_PROGRAM_LEN;
        printf("\r\nLoaded: 2-second timer\r\n");
    }
    else if (strcmp(cmd, "load echo") == 0) {
        active_program = echo_program;
        active_program_len = ECHO_PROGRAM_LEN;
        printf("\r\nLoaded: echo\r\n");
    }
    else if (strcmp(cmd, "load not") == 0) {
        active_program = not_gate_program;
        active_program_len = NOT_GATE_PROGRAM_LEN;
        printf("\r\nLoaded: NOT gate\r\n");
    }
    else if (strcmp(cmd, "load counter") == 0) {
        active_program = counter_demo_program;
        active_program_len = COUNTER_DEMO_PROGRAM_LEN;
        printf("\r\nLoaded: counter\r\n");
    }
    else if (strcmp(cmd, "reset") == 0) {
        for (int i = 0; i < PLC_DIGITAL_OUTPUT_COUNT; i++) {
            g_io.digital_out[i] = 0;
        }
        printf("\r\nI/O reset.\r\n");
    }
    else if (strlen(cmd) > 0) {
        printf("\r\nUnknown: '%s' (try 'help')\r\n", cmd);
    }
}

void uart_console_poll(void) {
    if (cmd_ready) {
        process_command(rx_buffer);
        rx_index = 0;
        cmd_ready = 0;
        printf("> ");
    }
}

const uint8_t *uart_get_active_program(void) {
    return active_program;
}

uint16_t uart_get_active_program_len(void) {
    return active_program_len;
}
