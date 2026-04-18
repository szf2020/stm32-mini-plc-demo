#include "scan_engine.h"
#include "main.h"
#include "io_image.h"

static uint32_t scan_count = 0;

void scan_engine_init(void) {
    scan_count = 0;
    io_image_init();
}

void scan_cycle_run(void) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);  // scan pulse

    scan_count++;

    // === PLC scan cycle ===

    // Phase 1: Read inputs
    io_read_inputs();

    // Phase 2: Execute ladder logic
    // -------------------------------------------------------------------
    // Motor start/stop with seal-in:
    //   Motor = (Start OR Motor) AND NOT Stop
    //
    //    Start         Stop       Motor
    //   --| |---+------|/|-------( )--
    //           |
    //    Motor  |
    //   --| |---+
    // -------------------------------------------------------------------
    bool start  = g_io.digital_in[0];
    bool stop   = g_io.digital_in[1];
    bool motor  = g_io.digital_out[0];   // current motor state

    g_io.digital_out[0] = (start || motor) && !stop;

    // Phase 3: Write outputs
    io_write_outputs();
}

uint32_t scan_get_count(void) {
    return scan_count;
}
