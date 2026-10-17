# matrix_game_freeRTOS

A structured, multi-file **FreeRTOS** application for the **Arduino UNO R4 WiFi** that animates the on-board **12x8 LED matrix**. The RTOS setup is encapsulated in a C++ class, the pixel data lives in a dedicated shared module, and the animation runs as an independent task.

This is the most advanced project of the repository: it moves from single-sketch experiments ([`freertos`](../freertos), [`blinkLedFreeRtos`](../blinkLedFreeRtos)) to a layered design with separated responsibilities.

> **Target board: Arduino UNO R4 WiFi.** This project was developed and tested only on the Arduino UNO R4 WiFi. It depends on the board's on-board 12x8 LED matrix and on the `Arduino_LED_Matrix` library of the UNO R4 core, so it will not run on other boards.

## Table of contents

1. [Overview](#overview)
2. [File structure](#file-structure)
3. [Architecture](#architecture)
4. [The frame buffer](#the-frame-buffer)
5. [Tasks](#tasks)
6. [Animation sequence](#animation-sequence)
7. [Execution flow](#execution-flow)
8. [Design choices](#design-choices)
9. [Hardware and libraries](#hardware-and-libraries)
10. [Build and run](#build-and-run)
11. [Technical notes](#technical-notes)
12. [Possible next steps](#possible-next-steps)

## Overview

The firmware drives the 96 LEDs of the UNO R4 WiFi matrix with a bit-level animation:

1. the matrix **fills up** one bit at a time,
2. it is set to **fully on**,
3. it **empties** one bit at a time,
4. the cycle repeats forever.

All timing is handled by FreeRTOS (`vTaskDelay`), so the animation never blocks the rest of the system.

## File structure

```
matrix_game_freeRTOS/
├── matrix_game_freeRTOS.ino   Entry point: global object + setup()/loop()
├── matrix_tasks.h             Declaration of the gestioneMatrice class and loop_func
├── matrix_task.cpp            Class implementation and the two FreeRTOS tasks
├── matrix_data.h              Frame type and extern declaration of the shared frame
└── matrix_data.cpp            Definition (storage) of the shared frame
```

| File | Responsibility |
|---|---|
| `matrix_game_freeRTOS.ino` | Creates the global `gestioneMatrice gMatrice` and calls `gMatrice.begin()` from `setup()`. `loop()` is intentionally empty |
| `matrix_tasks.h` | Public interface: `gestioneMatrice` class (`begin`, `task_frame`) and the `loop_func` task prototype |
| `matrix_task.cpp` | Owns the `ArduinoLEDMatrix` driver instance, the task handles, the class methods and both task bodies |
| `matrix_data.h` | Defines `struct Frame` and exposes the shared `frame` through `extern` |
| `matrix_data.cpp` | Allocates the single global `frame`, initialized to all zeros |

## Architecture

```
                        ┌─────────────────────────────┐
                        │ matrix_game_freeRTOS.ino    │
                        │  gestioneMatrice gMatrice   │
                        │  setup() → gMatrice.begin() │
                        └──────────────┬──────────────┘
                                       │
                        ┌──────────────▼──────────────┐
                        │ matrix_task.cpp             │
                        │  gestioneMatrice::begin()   │
                        │   ├─ matrix.begin()         │
                        │   ├─ create loop task       │
                        │   ├─ create frame task      │
                        │   └─ vTaskStartScheduler()  │
                        └───────┬──────────────┬──────┘
                                │              │
                  ┌─────────────▼───┐    ┌─────▼──────────────────┐
                  │ loop_func       │    │ task_frame(&frame)     │
                  │ loop(); yield   │    │ animates frame.dati    │
                  └─────────────────┘    │ matrix.loadFrame(...)  │
                                         └─────┬──────────────────┘
                                               │ reads / writes
                                      ┌────────▼─────────┐
                                      │ matrix_data      │
                                      │ Frame frame      │
                                      │ uint32_t dati[3] │
                                      └──────────────────┘
```

The code is split in three layers:

- **Application layer** (`.ino`): only instantiates the manager and starts it.
- **Logic layer** (`matrix_tasks.h` / `matrix_task.cpp`): task creation, scheduling and the animation algorithm.
- **Data layer** (`matrix_data.h` / `matrix_data.cpp`): the state shared by the tasks, kept apart from the logic that manipulates it.

## The frame buffer

The LED matrix has 12 columns x 8 rows = **96 pixels**, exactly 96 bits. The buffer is therefore stored as three 32-bit words:

```cpp
struct Frame
{
  uint32_t dati[3];
};
```

| Word | Bits | Pixels covered |
|---|---|---|
| `dati[0]` | 32 | first third of the matrix |
| `dati[1]` | 32 | second third |
| `dati[2]` | 32 | last third |

This is the same format expected by `ArduinoLEDMatrix::loadFrame(const uint32_t[3])`, so the buffer can be handed to the driver with no conversion.

The single instance is **defined** in `matrix_data.cpp` and **declared** `extern` in `matrix_data.h`, so every translation unit that includes the header sees the same object. A pointer to it is passed to the animation task through the FreeRTOS `pvParameters` argument, instead of letting the task reach for a global by name.

## Tasks

Both tasks are created with `xTaskCreate`, priority **1**, stack of `512/4` words (128 words = 512 bytes).

### `loop_func` (task `loop func`)

Runs the Arduino `loop()` inside FreeRTOS and calls `taskYIELD()` after each pass. This keeps the standard Arduino programming model available (anything placed in `loop()` becomes a task) without letting it monopolize the CPU. Since `loop()` is empty in this project, the task acts as a placeholder for future logic.

### `gestioneMatrice::task_frame` (task `thread 0 func`)

Receives the `Frame*` through `pvParameters`, casts it with `static_cast<Frame*>` and runs the animation loop forever. It is a `static` member function so it matches the plain C signature required by `xTaskCreate`, while still living inside the class namespace.

## Animation sequence

One full cycle is made of three phases, repeated indefinitely. The step time is **250 ms**.

| Phase | Operation | Steps | Duration |
|---|---|---|---|
| 1. Fill | Sets bit `n` (`\|= 1UL << n`) in all three words, waits 250 ms, then pushes the frame | 32 | about 8 s |
| 2. Full | Writes `0xFFFFFFFF` to all three words, pushes the frame, waits 250 ms | 3 | about 0.75 s |
| 3. Empty | Clears bit `n` (`&= ~(1UL << n)`) in all three words, pushes the frame, waits 250 ms | 33 | about 8.25 s |

Because the same bit index is changed in all three words at once, the three segments of the matrix evolve **in parallel**, producing a symmetric filling and emptying pattern.

The bit operations used:

| Goal | Expression |
|---|---|
| Turn a pixel on | `word \|= (1UL << bit)` |
| Turn a pixel off | `word &= ~(1UL << bit)` |
| Turn everything on | `word = 0xFFFFFFFF` |
| Turn everything off | `word = 0x00000000` |

`1UL` is used instead of `1` so the shift is performed on an unsigned 32-bit value and never overflows into the sign bit.

## Execution flow

1. Power-up: the global `gMatrice` is constructed, which configures pin 13 (`LED_BI`) as output.
2. `setup()` calls `gMatrice.begin()`.
3. `begin()` initializes the LED matrix driver, creates the two tasks and starts the FreeRTOS scheduler.
4. The scheduler never returns: from here on, the system lives inside the tasks.
5. `task_frame` loops over fill, full and empty phases; `loop_func` spins on the (empty) Arduino `loop()`.

## Design choices

- **RTOS wrapped in a class.** The scheduler startup and task creation are hidden behind a single `begin()` call, so the sketch stays three lines long.
- **Static member as task entry point.** FreeRTOS needs a function with no hidden `this` pointer; `static` provides it while keeping the task private to the class.
- **Data separated from logic.** `matrix_data` only knows what a frame is; `matrix_task` knows how to animate it. Changing the animation does not touch the data module, and vice versa.
- **Parameter passing instead of implicit globals.** The frame is injected into the task via `pvParameters`, which makes the task reusable with any buffer.
- **Time-based, non-blocking waits.** `vTaskDelay(pdMS_TO_TICKS(250))` puts the task in the blocked state, leaving the CPU to the other task.
- **Header guards** on every header to prevent duplicate definitions.

## Hardware and libraries

- **Board:** Arduino UNO R4 WiFi (12x8 LED matrix on board)
- **Libraries:**
  - `Arduino_FreeRTOS`
  - `Arduino_LED_Matrix` (part of the UNO R4 core)

No external wiring is needed.

## Build and run

1. Install the **Arduino UNO R4 Boards** core and the **Arduino_FreeRTOS** library.
2. Open `matrix_game_freeRTOS/matrix_game_freeRTOS.ino` in the Arduino IDE (all the other files in the folder are compiled together automatically).
3. Select **Arduino UNO R4 WiFi** and the serial port.
4. Upload. The matrix starts filling right away; no Serial Monitor is required.

## Technical notes

Observations on the current implementation, useful when extending it:

- **Shift by 32 in the empty phase.** The last loop starts from `bit = 32`, while valid shifts on a 32-bit value are 0 to 31. In C++ a shift by 32 is undefined behavior; starting from `31` would give the intended 32 steps (and 250 ms less per cycle).
- **Phase 2 is not a blink.** Inside the loop the buffer is cleared and then immediately overwritten with `0xFFFFFFFF` before being displayed, so the three iterations show a steady full matrix. A real flash would push the all-zero frame to the matrix and wait before setting it to all-ones.
- **Display update order in phase 1.** The frame is pushed after the 250 ms delay, so each new pixel appears one step later than the bit is set. Pushing before the delay would remove the latency.
- **Shared state without a lock.** Today only `task_frame` touches the frame, so no synchronization is needed. As soon as a second task writes to it, protect it with a mutex (`xSemaphoreCreateMutex`) or move updates behind a queue.
- **Pin setup in the constructor.** `pinMode` runs when the global object is constructed, before `setup()`. Moving it into `begin()` guarantees the board is fully initialized when it executes.
- **Unused objects.** `int bit` in the `.ino` and the `LED_BI` pin are not used by the animation yet; they are leftovers from the previous projects.
- **`vTaskStartScheduler()` inside `setup()`** never returns, so `loop()` of the sketch is never called directly by the Arduino core: it is only run by `loop_func`.
- **Stack budget.** 512 bytes per task is enough for the current code. Use `uxTaskGetStackHighWaterMark` if more local variables or calls are added.

## Possible next steps

- Use the buffer for real game logic (a moving pixel, snake, pong) driven by buttons or the serial port.
- Add a mutex or a queue between input and display tasks.
- Separate display refresh from game logic into two tasks with different priorities.
- Add runtime statistics with `vTaskGetInfo` / `uxTaskGetStackHighWaterMark`.
- Replace the fixed 250 ms with `vTaskDelayUntil` for a drift-free frame rate.
