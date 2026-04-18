#include "scan_engine.h"
#include "main.h"
#include "io_image.h"

static uint32_t scan_count = 0;

void scan_engine_init(void) {
    scan_count = 0;
    io_image_init();
}

void scan_cycle_run(void) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);  // scan pulse for logic analyzer

    scan_count++;

    // === 3-phase PLC scan cycle ===

    // Phase 1: Read inputs
    io_read_inputs();

    // Phase 2: Execute logic (dummy logic for now)
    // Toggle output 0 every 50 scans = every 500 ms
    if ((scan_count % 50) == 0) {
        g_io.digital_out[0] = !g_io.digital_out[0];
    }

    // Phase 3: Write outputs
    io_write_outputs();
}

uint32_t scan_get_count(void) {
    return scan_count;
}
