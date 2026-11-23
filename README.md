# NHUNGDOXE - Du an MCU + FreeRTOS

## Cau truc thu muc
| Thu muc        | Noi dung                                          |
|----------------|---------------------------------------------------|
| `bsp/`         | Board support & pin mapping                       |
| `hal/inc,src`  | Driver ngoai vi: GPIO, UART, I2C, SPI, Timer, ADC |
| `middlewares/` | FreeRTOS source (Kernel, Port, Heap)              |
| `app/inc,src`  | FreeRTOS Tasks & IPC                              |
| `main.c`       | Init hardware, tao task, vTaskStartScheduler()    |

## Chien luoc nhanh
- `main`: code chay on dinh tren kit. **Cam commit truc tiep.**
- `develop`: nhanh tich hop chinh. Moi nhanh con tao tu day va merge ve day.
- `feature/<ten-task>`: nhanh lam viec rieng cua tung nguoi.
- `integration/<phien-ban>`: ghep noi he thong truoc khi release.

## Quy trinh lam viec
1. `git checkout develop` va `git pull origin develop`
2. `git checkout -b feature/<ten-task>`
3. Code, test tren kit
4. `git add <file>` va `git commit -m "feat(...): mo ta"`
5. `git push -u origin feature/<ten-task>`
6. Mo Pull Request vao `develop`, can 1 nguoi Approve truoc khi merge.

## Quy tac commit message
- `feat(task-xxx): ...` them chuc nang task hoac IPC moi
- `fix(rtos): ...` sua loi stack, priority, deadlock, HAL driver
- `refactor(...): ...` toi uu cau truc code

## Quy chuan bat buoc
1. Chi **1 nguoi** cau hinh CubeMX/IDE va generate code. Khong ai khac sua `FreeRTOSConfig.h`.
2. Trong ISR chi dung API `...FromISR()`.
3. Trong task dung `vTaskDelay()` / `vTaskDelayUntil()`, khong dung `HAL_Delay()` / `delay_ms()`.
4. Khong commit `build/`, `Debug/`, `Release/` va file tam cua IDE.
5. Khong sua `main.c` khi dang lam task rieng (chi sua o giai doan Integration).
