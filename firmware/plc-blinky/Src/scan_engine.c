#include "scan_engine.h"
#include "main.h"
#include "io_image.h"
#include "ladder_vm.h"
#include "ladder_programs.h"
#include "timer_counter.h"
#include "uart_console.h"
static uint32_t scan_count = 0;

void scan_engine_init(void) {
    scan_count = 0;
    io_image_init();
    timer_counter_init();
}

void scan_cycle_run(void) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);  // scan pulse

    scan_count++;

    // === PLC scan cycle ===

    // Phase 1: Read inputs
    io_read_inputs();

    //ladder_vm_execute(echo_program, ECHO_PROGRAM_LEN);
    // Phase 2: Execute ladder program through the VM
    // Phase 2: Execute the program currently selected via UART console
    ladder_vm_execute(uart_get_active_program(), uart_get_active_program_len());
    //ladder_vm_execute(motor_start_stop_program, MOTOR_PROGRAM_LEN);
    //ladder_vm_execute(timer_demo_program, TIMER_DEMO_PROGRAM_LEN);
    //ladder_vm_execute(not_gate_program, NOT_GATE_PROGRAM_LEN);
    //ladder_vm_execute(counter_demo_program, COUNTER_DEMO_PROGRAM_LEN);
    //ladder_vm_execute(echo_program, ECHO_PROGRAM_LEN);
    // Phase 3: Write outputs
    io_write_outputs();
}

uint32_t scan_get_count(void) {
    return scan_count;
}
