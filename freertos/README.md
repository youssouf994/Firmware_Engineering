# freertos

First experiment with **FreeRTOS** on the **Arduino UNO R4 WiFi**: two concurrent tasks, one that keeps the classic Arduino `loop()` alive and one that blinks the on-board LED.

> **Target board: Arduino UNO R4 WiFi.** This project was developed and tested only on the Arduino UNO R4 WiFi.

## What it does

- Opens the serial port at 115200 baud and waits for a serial monitor to connect.
- Creates two tasks with the same priority (1) and a 512-byte stack each:
  - `loop thread`: calls the Arduino `loop()` forever and yields the CPU after each call.
  - `led 2 thread`: toggles the LED on pin 13 every 150 ms.
- Starts the FreeRTOS scheduler.

## How it works

| Piece | Role |
|---|---|
| `setup()` | Initializes serial and the LED pin, creates the tasks, prints `starting scheduler`, starts the scheduler |
| `loop()` | Empty on purpose; it is executed inside `loop_thread_func` so the Arduino programming model still works under FreeRTOS |
| `loop_thread_func` | Wraps `loop()` in an infinite cycle and calls `taskYIELD()` so the other task can run |
| `led2_thread_func` | Toggles the LED with `digitalWrite(LED1, !digitalRead(LED1))` and sleeps with `vTaskDelay(pdMS_TO_TICKS(150))` |

The stack size is written as `512/4` because FreeRTOS expresses it in **words** (4 bytes on ARM Cortex-M), so the result is 128 words = 512 bytes.

## Hardware

- Arduino UNO R4 WiFi
- On-board LED (pin 13), no external wiring required

## Libraries

- `Arduino_FreeRTOS`
- `Arduino_LED_Matrix` (included, but the matrix is not used in this sketch)

## Run it

1. Open `freertos/freertos.ino` in the Arduino IDE.
2. Select **Arduino UNO R4 WiFi**.
3. Upload and open the Serial Monitor at **115200** baud (the sketch waits for it).
4. The on-board LED starts blinking.

## Notes

- `vTaskStartScheduler()` never returns, so the code after it (`matrice.begin()`) is never reached. It is a leftover of the first experiments and is the natural starting point of the next projects.
- `while (!Serial)` blocks the boot until the Serial Monitor is opened.
- This sketch is the starting point of [`blinkLedFreeRtos`](../blinkLedFreeRtos) and, later, of [`matrix_game_freeRTOS`](../matrix_game_freeRTOS).
