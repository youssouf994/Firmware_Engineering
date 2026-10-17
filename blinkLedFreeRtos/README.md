# blinkLedFreeRtos

A **FreeRTOS** LED blink on the **Arduino UNO R4 WiFi**, built with two cooperating tasks: one hosting the Arduino `loop()` and one dedicated to toggling the on-board LED.

> **Target board: Arduino UNO R4 WiFi.** This project was developed and tested only on the Arduino UNO R4 WiFi.

## What it does

- Initializes the serial port at 115200 baud and waits for the Serial Monitor.
- Creates two priority-1 tasks, each with a 512-byte stack:
  - `loop thread`: runs `loop()` continuously and yields after every iteration.
  - `led 2 thread`: inverts the state of the LED on pin 13 every **150 ms**.
- Starts the scheduler, which takes control of the board.

## How it works

```
setup()
  ├─ Serial.begin(115200) ── wait for monitor
  ├─ pinMode(LED1, OUTPUT)
  ├─ xTaskCreate(loop_thread_func)
  ├─ xTaskCreate(led2_thread_func)
  └─ vTaskStartScheduler()

loop thread  : loop() → taskYIELD() → loop() → ...
led 2 thread : toggle LED → vTaskDelay(150 ms) → toggle LED → ...
```

- **Blocking delay vs. yield:** the LED task sleeps with `vTaskDelay`, letting the scheduler run something else; the loop task never sleeps and only gives up the CPU voluntarily with `taskYIELD()`.
- **Time conversion:** `pdMS_TO_TICKS(150)` converts milliseconds into scheduler ticks, so the code does not depend on the tick rate.
- **Stack size:** `512/4` words = 128 words = 512 bytes per task.

## Hardware

- Arduino UNO R4 WiFi, using the on-board LED on pin 13. No extra components.

## Libraries

- `Arduino_FreeRTOS`
- `Arduino_LED_Matrix` (declared but not used here)

## Run it

1. Open `blinkLedFreeRtos/blinkLedFreeRtos.ino` in the Arduino IDE.
2. Select **Arduino UNO R4 WiFi** and the serial port.
3. Upload and open the Serial Monitor at **115200** baud.
4. The LED blinks at roughly 3.3 Hz (150 ms per state).

## Notes

- The code after `vTaskStartScheduler()` is unreachable, since the scheduler never returns.
- The sketch is currently identical to [`freertos`](../freertos); it is kept as the clean reference for the blink task pattern reused in [`matrix_game_freeRTOS`](../matrix_game_freeRTOS).
