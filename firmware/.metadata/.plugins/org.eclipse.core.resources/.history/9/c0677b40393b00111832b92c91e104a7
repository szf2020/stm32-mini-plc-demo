#include "io_image.h"
#include "main.h"
#include <string.h>

// The global I/O image instance
plc_io_image_t g_io;

void io_image_init(void) {
    memset(&g_io, 0, sizeof(g_io));
}

void io_read_inputs(void) {
    // For now: no real inputs wired yet.
    // On Day 3 we'll wire 8 GPIO pins and read them here.
    // Leaving digital_in[] at zero for now.

    // Future code will look like:
    // g_io.digital_in[0] = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET);
    // g_io.digital_in[1] = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET);
    // etc.
}

void io_write_outputs(void) {
    // Drive onboard LED (PC13) from digital_out[0] for now.
    // Day 3: wire 4 relays + 4 transistor outputs to 8 GPIO pins.
    //
    // Note: PC13 on Blue Pill is ACTIVE-LOW — LED lights when pin is LOW.
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
                      g_io.digital_out[0] ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
