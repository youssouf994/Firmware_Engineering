# matrix_game_multi_task

A **multi-task FreeRTOS** application for the **Arduino UNO R4 WiFi** that drives the on-board **12x8 LED matrix** with several independent tasks at once. Three "pattern" tasks each own a slice of the frame and animate it with their own timing, while a separate **loader task** periodically pushes the shared frame to the display.

It is the natural follow-up of [`matrix_game_freeRTOS`](../../tree/matrix_game_freeRTOS): there a single task animated the whole frame, here the work is split across tasks that cooperate through a shared data contract.

> **Target board: Arduino UNO R4 WiFi.** This project was developed and tested only on the Arduino UNO R4 WiFi. It depends on the board's on-board 12x8 LED matrix and on the `Arduino_LED_Matrix` library of the UNO R4 core, so it will not run on other boards.

> **Work in progress: the light games are still to be defined.** The three patterns in this sketch (blink, inverted blink, bit-by-bit fill) are a *test bench* for the multi-task architecture, not the final animations. The real light games will be designed later and will be the practical side of the embedded theory this repository is going to cover: **bit shifting, bitwise logical operators, masks** and related low-level techniques. See [Roadmap: light games and bit manipulation](#roadmap-light-games-and-bit-manipulation).

## Table of contents

1. [Overview](#overview)
2. [File structure](#file-structure)
3. [Architecture](#architecture)
4. [The data contract](#the-data-contract)
5. [Tasks](#tasks)
6. [Timeline of one run](#timeline-of-one-run)
7. [Scheduling and priorities](#scheduling-and-priorities)
8. [Shared data and concurrency](#shared-data-and-concurrency)
9. [Differences from matrix_game_freeRTOS](#differences-from-matrix_game_freertos)
10. [Roadmap: light games and bit manipulation](#roadmap-light-games-and-bit-manipulation)
11. [Hardware and libraries](#hardware-and-libraries)
12. [Build and run](#build-and-run)
13. [Technical notes](#technical-notes)
14. [Possible next steps](#possible-next-steps)

## Overview

The 96 LEDs of the matrix are stored in one shared frame made of three 32-bit words. Each word is handed to a different task:

| Word | Task | Behavior |
|---|---|---|
| `data[0]` | `func_task_frame1` | Blinks: all on for 750 ms, all off for 750 ms |
| `data[1]` | `func_task_frame2` | Fills bit by bit, one pixel every 250 ms, then restarts from empty |
| `data[2]` | `func_task_frame3` | Blinks in **opposition** to `data[0]`: off for 750 ms, then on for 750 ms |

A fourth task, `carica_frame`, sends the whole frame to the matrix every 20 ms. The pattern tasks never talk to the display: they only write their own word, the loader does the rest. This separation between *producing* the data and *showing* it is the main idea of the project.

## File structure

```
matrix_game_multi_task/
├── matrix_game_multi_task.ino   Entry point: global object + setup()/loop()
├── matrix_tasks.h               Declaration of the gestore_tasks class
├── matrix_tasks.cpp             Class implementation and the four FreeRTOS tasks
├── matrix_data.h                Message and Contract types, extern declarations
└── matrix_data.cpp              Definition (storage) of the shared frame and the contracts
```

| File | Responsibility |
|---|---|
| `matrix_game_multi_task.ino` | Creates the global `gestore_tasks scheduler` and calls `scheduler.begin()` from `setup()`. `loop()` is intentionally empty |
| `matrix_tasks.h` | Public interface of `gestore_tasks`: `begin()` and the three pattern tasks. The loader `carica_frame` is `private` |
| `matrix_tasks.cpp` | Owns the `ArduinoLEDMatrix` driver, the task handles, the boot test and the four task bodies |
| `matrix_data.h` | Defines `struct Message` (the frame) and `struct Contract` (buffer pointer plus word index) |
| `matrix_data.cpp` | Allocates the single global `valours` frame and the array of three `Contract` |

## Architecture

```
                     ┌──────────────────────────────────┐
                     │ matrix_game_multi_task.ino       │
                     │  gestore_tasks scheduler         │
                     │  setup() → scheduler.begin()     │
                     └────────────────┬─────────────────┘
                                      │
                     ┌────────────────▼─────────────────┐
                     │ matrix_tasks.cpp                 │
                     │  gestore_tasks::begin()          │
                     │   ├─ matrix.begin()              │
                     │   ├─ boot test pattern (3 s)     │
                     │   ├─ create 3 pattern tasks      │
                     │   ├─ create loader task          │
                     │   └─ vTaskStartScheduler()       │
                     └──┬─────────┬─────────┬─────────┬─┘
                        │         │         │         │
              ┌─────────▼──┐ ┌────▼─────┐ ┌─▼────────┐│
              │ frame1     │ │ frame2   │ │ frame3   ││ prio 2
              │ writes     │ │ writes   │ │ writes   ││
              │ data[0]    │ │ data[1]  │ │ data[2]  ││
              └─────┬──────┘ └────┬─────┘ └────┬─────┘│
                    │ write       │ write      │ write│
                    └─────────────┼────────────┘      │
                          ┌───────▼────────┐   ┌──────▼──────────┐
                          │ matrix_data    │◄──│ carica_frame    │ prio 4
                          │ Message valours│   │ loadFrame(...)  │ every 20 ms
                          │ uint32_t[3]    │   └─────────────────┘
                          └────────────────┘
```

The code is split in three layers, as in the previous project:

- **Application layer** (`.ino`): only instantiates the manager and starts it.
- **Logic layer** (`matrix_tasks.h` / `matrix_tasks.cpp`): task creation, scheduling and the animation algorithms.
- **Data layer** (`matrix_data.h` / `matrix_data.cpp`): the state shared by the tasks.

## The data contract

```cpp
struct Message            // the frame
{
  uint32_t data[3];       // 3 x 32 bits = 96 pixels
};

struct Contract           // what a task receives
{
  Message *buffer;        // pointer to the shared frame
  uint8_t  array_position;// which word of the frame the task owns
};
```

The 12x8 matrix has 96 pixels, exactly 96 bits, which is the format expected by `ArduinoLEDMatrix::loadFrame(const uint32_t[3])`: the shared frame can be given to the driver with no conversion.

`matrix_data.cpp` defines one frame and three contracts that **all point to the same frame**, each with a different word index:

```cpp
Message  valours;                       // all zeros
Contract contract[3] = { {&valours, 0}, {&valours, 1}, {&valours, 2} };
```

A task receives `&contract[n]` through the FreeRTOS `pvParameters` argument and from it knows two things: *where* the frame is and *which word is its own*. The task code does not contain any hard-coded index, so the same function could be reused on a different word by passing a different contract.

## Tasks

All tasks are created with `xTaskCreate` and a stack of `512/4` words (128 words = 512 bytes, FreeRTOS counts the stack in words of 4 bytes).

| Task name | Function | Priority | Parameter | Period |
|---|---|---|---|---|
| `frame 1` | `func_task_frame1` | 2 | `&contract[0]` | 1500 ms (750 on + 750 off) |
| `task 2` | `func_task_frame2` | 2 | `&contract[1]` | 8000 ms (32 steps x 250 ms) |
| `task 3` | `func_task_frame3` | 2 | `&contract[2]` | 1500 ms (750 off + 750 on) |
| `loader` | `carica_frame` | 4 | `&contract[0]` | 20 ms (about 50 refreshes per second) |

### `func_task_frame1` and `func_task_frame3`: blink and inverted blink

Each one writes `0xFFFFFFFF` (all 32 pixels on) and `0x00000000` (all off) to its own word, separated by `vTaskDelay(pdMS_TO_TICKS(750))`. They are the mirror image of each other: when the first word is lit the third is dark, and vice versa.

### `func_task_frame2`: bit-by-bit fill

Takes a pointer to its own word and, at every iteration of the outer loop, clears it and then sets one more bit every 250 ms:

```cpp
*w = 0;
for (int bit = 0; bit < 32; bit++)
{
  *w |= (1UL << bit);          // turn on pixel number "bit"
  vTaskDelay(pdMS_TO_TICKS(250));
}
```

`1UL << bit` builds a mask with a single bit set; the OR `|=` turns that pixel on without disturbing the others. After 32 steps the word is full for 250 ms and then the outer loop clears it and starts again.

### `carica_frame`: the loader

Calls `matrix.loadFrame(m->buffer->data)` every 20 ms. It is the **only** task that talks to the display driver, which keeps the pattern tasks independent from the hardware. It receives `&contract[0]` but only uses `buffer`, the word index is irrelevant here.

## Timeline of one run

1. **Power-up**: the global `scheduler` is constructed (the constructor is empty).
2. `setup()` calls `scheduler.begin()`.
3. `begin()` initializes the matrix driver and writes a **boot test pattern** to the frame: one byte lit in each word (`0x000000FF`, `0x0000FF00`, `0x00FF0000`). The frame is shown for **3 seconds** using the plain Arduino `delay(3000)`, which is fine here because the scheduler has not started yet.
4. The three pattern tasks and the loader are created and `vTaskStartScheduler()` is called. It never returns.
5. From now on the patterns overwrite the boot test and the loader keeps the display up to date.

## Scheduling and priorities

FreeRTOS always runs the ready task with the highest priority. Here the loader has priority **4** and the pattern tasks priority **2**, so whenever the loader is ready it preempts the others. This is safe only because every task spends almost all of its time in the *blocked* state:

- the loader blocks for 20 ms after each refresh (a `loadFrame` call takes a tiny fraction of that);
- the pattern tasks block on `vTaskDelay` between two writes.

While everything is blocked, the scheduler runs the idle task. If a task ever looped without a delay, it would starve all the tasks of lower priority, so a blocking call is mandatory in every `for(;;)`.

`pdMS_TO_TICKS(ms)` converts milliseconds to scheduler ticks, so the timing does not depend on the configured tick rate.

## Shared data and concurrency

The three pattern tasks and the loader access the same `Message`, but with a clear rule: **each pattern task writes only its own word, the loader only reads**. That has two consequences:

- two tasks never write the same word, so there is no write/write conflict;
- on the Cortex-M4 an aligned 32-bit store is a single instruction, so the loader never sees a half-written word.

What can happen is that the loader reads the three words at slightly different moments and shows, for one refresh (20 ms), a frame that mixes "old" and "new" values from different words. For these animations this is invisible. If a future game needs the three words to be **consistent with each other**, protect the frame with a mutex (`xSemaphoreCreateMutex`) or pass updates through a queue.

## Differences from matrix_game_freeRTOS

| | `matrix_game_freeRTOS` | `matrix_game_multi_task` |
|---|---|---|
| Animation tasks | 1 (`task_frame`) | 3, one per word of the frame |
| Display update | Done by the animation task itself | Done by a dedicated loader task |
| Parameter passed to tasks | Pointer to the frame | Pointer to a `Contract` (frame + owned word) |
| Priorities | All equal (1) | Patterns 2, loader 4 |
| Arduino `loop()` task | Wrapped in a task (`loop_func`) | Not used, `loop()` stays empty |
| Boot behavior | Starts animating immediately | 3 s test pattern, then tasks start |
| Source of the timing | Single 250 ms step | Independent periods per task |

## Roadmap: light games and bit manipulation

The animations in this project are a **placeholder**. The actual light games are **to be defined**. The plan is to use them as the hands-on part of the theory of the embedded programming topics below, one concept at a time, on top of the architecture built here:

| Topic | What it is | Where it will show up |
|---|---|---|
| **Bit shift** (`<<`, `>>`) | Moving the bits of a value left or right; `1UL << n` builds a mask with only bit `n` set | Scrolling a pattern, moving a pixel across the matrix |
| **Bitwise OR** (`\|`) | Sets bits | Turning pixels on (`word \|= mask`), already used by `func_task_frame2` |
| **Bitwise AND** (`&`) | Isolates or clears bits | Testing a pixel, clearing pixels, collision detection |
| **Bitwise XOR** (`^`) | Flips bits | Toggling pixels, blinking without knowing the previous state |
| **Bitwise NOT** (`~`) | Inverts all bits | Inverting a whole frame, building clear masks (`word &= ~mask`) |
| **Masks and bit fields** | Selecting a group of bits | Addressing a row or a column inside the 96-bit frame |
| **Set / clear / toggle / test a bit** | The four basic operations on a single bit | The building blocks of every game on the matrix |
| **Hexadecimal and binary notation** | Reading and writing values such as `0x00FF0000` | Describing frames and patterns compactly |
| **Packing data across words** | Mapping a (row, column) position to a word and a bit index | Treating the three `uint32_t` as a 12x8 grid |
| **Register-level thinking** | Configuring hardware by setting and clearing bits in registers | Connecting the matrix techniques to how microcontroller peripherals are configured |

The topics are listed here as a plan: they will be added, explained and implemented progressively, and this README will be updated as each one lands.

## Hardware and libraries

- **Board:** Arduino UNO R4 WiFi (12x8 LED matrix on board)
- **Libraries:**
  - `Arduino_FreeRTOS`
  - `Arduino_LED_Matrix` (part of the UNO R4 core)

No external wiring is needed.

## Build and run

1. Install the **Arduino UNO R4 Boards** core (the FreeRTOS and LED matrix libraries come with it; install `Arduino_FreeRTOS` from the Library Manager if the compiler cannot find it).
2. Open `matrix_game_multi_task/matrix_game_multi_task.ino` in the Arduino IDE. All the other files in the folder are compiled together automatically. The sketch folder and the `.ino` must have the same name, as they do here.
3. Select **Arduino UNO R4 WiFi** and the serial port.
4. Upload. First you see the 3-second boot test, then the three patterns run in parallel. No Serial Monitor is required.

The sketch compiles for `arduino:renesas_uno:unor4wifi` (about 23% of the flash and 47% of the RAM).

## Technical notes

Observations on the current implementation, useful when extending it:

- **Redundant initialization.** `begin()` sets `array_position` of each contract again, but `matrix_data.cpp` already initializes it to 0, 1, 2. One of the two can be removed.
- **Loader receives more than it needs.** `carica_frame` gets `&contract[0]` but only reads `buffer`. Passing `&valours` (or a pointer to the frame) would make the intent clearer.
- **No hold on the full word.** `func_task_frame2` clears the word at the beginning of each cycle, so the completely lit state lasts one step (250 ms) and the empty state is never held: the pattern restarts immediately.
- **No synchronization.** Safe today because every word has a single writer. See [Shared data and concurrency](#shared-data-and-concurrency) before adding a second writer to the same word.
- **Task names are inconsistent** (`frame 1`, `task 2`, `task 3`, `loader`). They only matter for debugging, but a uniform scheme (`frame1`, `frame2`, `frame3`, `loader`) helps when inspecting tasks.
- **File naming.** The implementation file is `matrix_tasks.cpp` here, while the previous project uses `matrix_task.cpp`.
- **`vTaskStartScheduler()` inside `setup()`** never returns, so the sketch `loop()` is never executed.
- **Stack budget.** 512 bytes per task is enough for the current code. Use `uxTaskGetStackHighWaterMark` if more local variables or calls are added.

## Possible next steps

- Define the first real light game and the rule set for the three words.
- Move from "one word per task" to a (row, column) addressing helper built on bit shifts and masks.
- Add input (buttons or serial commands) as another task that feeds the game logic.
- Protect the frame with a mutex or a queue when more than one task has to write the same word.
- Replace the fixed delays with `vTaskDelayUntil` for drift-free periods.
- Add runtime statistics with `uxTaskGetStackHighWaterMark` and `vTaskGetInfo`.
