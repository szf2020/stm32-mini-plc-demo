#include "io_image.h"
#include "main.h"
#include <string.h>

// The global I/O image instance
plc_io_image_t g_io;

void io_image_init(void) {
    memset(&g_io, 0, sizeof(g_io));
}
void io_read_inputs(void) {
    // Input 0: industrial 24V input via optocoupler (active LOW)
    g_io.digital_in[0] = (HAL_GPIO_ReadPin(IND_IN_0_GPIO_Port, IND_IN_0_Pin) == GPIO_PIN_RESET);

    // Input 1: keep the button on PB1 as Stop
    g_io.digital_in[1] = (HAL_GPIO_ReadPin(STOP_BTN_GPIO_Port, STOP_BTN_Pin) == GPIO_PIN_RESET);

    for (int i = 2; i < PLC_DIGITAL_INPUT_COUNT; i++) {
        g_io.digital_in[i] = false;
    }
}
void io_write_outputs(void) {
    // Output 0: industrial output via relay driver
    HAL_GPIO_WritePin(IND_OUT_0_GPIO_Port, IND_OUT_0_Pin,
                      g_io.digital_out[0] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Also keep PC13 mirroring the output for visual feedback
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
                      g_io.digital_out[0] ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
