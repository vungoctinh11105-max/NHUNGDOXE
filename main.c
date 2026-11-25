#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "hal_gpio.h"
#include "hal_uart.h"
#include "app_ipc.h"

int main(void)
{
    /* 1. Hardware & HAL init */
    bsp_system_init();
    hal_gpio_init();
    hal_uart_init(115200);

    /* 2. IPC - hoan thien o giai doan Integration */
    /* app_ipc_init(); */

    /* 3. Create tasks - them o giai doan Integration */
    /* task_sensor_create(); task_display_create(); task_comm_create(); */

    /* 4. Start scheduler */
    vTaskStartScheduler();

    for (;;);
}
