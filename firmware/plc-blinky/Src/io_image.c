#include "io_image.h"
#include "main.h"
#include <string.h>

// The global I/O image instance
plc_io_image_t g_io;

void io_image_init(void) {
    memset(&g_io, 0, sizeof(g_io));
}

void io_read_inputs(void) {
    // Input 0: Start button (active-LOW, pressed = pin grounded)
    // Input 1: Stop button (active-LOW)
    //
    // With pull-ups enabled:
    //   Pin HIGH (not pressed) = HAL reads GPIO_PIN_SET
    //   Pin LOW  (pressed)     = HAL reads GPIO_PIN_RESET
    //
    // We invert so that "pressed" = true in our image.

    g_io.digital_in[0] = (HAL_GPIO_ReadPin(START_BTN_GPIO_Port, START_BTN_Pin) == GPIO_PIN_RESET);
    g_io.digital_in[1] = (HAL_GPIO_ReadPin(STOP_BTN_GPIO_Port, STOP_BTN_Pin) == GPIO_PIN_RESET);

    // Remaining digital inputs stay at zero (not wired yet)
    for (int i = 2; i < PLC_DIGITAL_INPUT_COUNT; i++) {
        g_io.digital_in[i] = false;
    }
}

void io_write_outputs(void) {
    // Drive onboard LED (PC13) from digital_out[0] for now.
    // Day 3: wire 4 relays + 4 transistor outputs to 8 GPIO pins.
    //
    // Note: PC13 on Blue Pill is ACTIVE-LOW — LED lights when pin is LOW.
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
                      g_io.digital_out[0] ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
