#include "scan_engine.h"
#include "main.h"
#include "io_image.h"
#include "ladder_vm.h"
#include "ladder_programs.h"
#include "timer_counter.h"
#include "uart_console.h"

static uint32_t scan_count = 0;
static uint8_t plc_run_mode = 1;  // Start in RUN mode by default

void scan_engine_init(void) {
    scan_count = 0;
    io_image_init();
    timer_counter_init();
}

void scan_cycle_run(void) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);  // scan pulse

    scan_count++;

    // === Check physical START / STOP buttons ===
    // Buttons are active LOW with internal pull-up
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
        plc_run_mode = 1;  // START pressed → RUN mode
    }
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) {
        plc_run_mode = 0;  // STOP pressed → STOP mode
        // Force all outputs OFF immediately
        for (int i = 0; i < 4; i++) {
            g_io.digital_out[i] = 0;
        }
    }

    // === PLC scan cycle ===

    // Phase 1: Read inputs
    io_read_inputs();

    // Phase 2: Execute program ONLY if in RUN mode
    if (plc_run_mode) {
        ladder_vm_execute(uart_get_active_program(),
                          uart_get_active_program_len());
    }
    // If in STOP mode, skip program execution (outputs already forced OFF above)

    // Phase 3: Write outputs
    io_write_outputs();
}

uint32_t scan_get_count(void) {
    return scan_count;
}

uint8_t scan_get_run_mode(void) {
    return plc_run_mode;
}
