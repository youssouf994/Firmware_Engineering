# Firmware Engineering

Hands-on firmware projects for the **Arduino UNO R4 WiFi**, learning embedded programming with **FreeRTOS**. The path goes from a first two-task experiment to multi-task applications that drive the on-board LED matrix, and it is meant to grow into a practical companion for the core low-level topics of embedded development.

> **Target board: Arduino UNO R4 WiFi.** Everything here was written, built and tested only on this board. The projects use its on-board LED matrix and the UNO R4 core, so they are not guaranteed to work on other boards (for example the classic UNO R3).

## Table of contents

1. [Projects](#projects)
2. [How the repository is organized](#how-the-repository-is-organized)
3. [Getting started](#getting-started)
4. [Hardware and software](#hardware-and-software)
5. [Concepts covered](#concepts-covered)
6. [Roadmap](#roadmap)
7. [Author](#author)

## Projects

Suggested reading order, from easiest to most advanced:

| # | Project | What you learn | Level |
|---|---|---|---|
| 1 | [`freertos`](../../tree/freertos) | Two tasks: one blinks the on-board LED, one wraps the Arduino `loop()` | Beginner |
| 2 | [`blinkLedFreeRtos`](../../tree/blinkLedFreeRtos) | Refined version of the same blink experiment | Beginner |
| 3 | [`matrix_game_freeRTOS`](../../tree/matrix_game_freeRTOS) | Class-based application animating the 12x8 LED matrix with one task and a shared frame buffer | Advanced |
| 4 | [`matrix_game_multi_task`](../../tree/matrix_game_multi_task) | Several tasks writing different parts of the same frame, plus a dedicated task that refreshes the display | Advanced |

Each project has its own `README.md` with a detailed explanation. Projects 3 and 4 are the in-depth ones: they describe the architecture, the data flow, the scheduling and the known limitations of the code.

## How the repository is organized

Every project lives in its **own branch**, named after the project, and contains only that project. The branch is the place where the project is developed and where its README is always up to date. Finished projects are then merged into `main`, which collects them together with this overview.

| Branch | Content |
|---|---|
| `main` | This README (overview and index) and the projects merged so far |
| `freertos` | The `freertos` project |
| `blinkLedFreeRtos` | The `blinkLedFreeRtos` project |
| `matrix_game_freeRTOS` | The `matrix_game_freeRTOS` project |
| `matrix_game_multi_task` | The `matrix_game_multi_task` project |

If a project folder is missing from `main`, it has not been merged yet: check out its branch.

## Getting started

1. Clone the repository and switch to the project you want:

   ```bash
   git clone https://github.com/youssouf994/Firmware_Engineering.git
   cd Firmware_Engineering
   git checkout matrix_game_multi_task   # or freertos, blinkLedFreeRtos, matrix_game_freeRTOS
   ```

2. Open the `.ino` file of the project in the Arduino IDE (the sketch folder and the `.ino` share the same name).
3. Select **Arduino UNO R4 WiFi** as the board and the correct serial port.
4. Install the `Arduino_FreeRTOS` library from the Library Manager if it is missing.
5. Upload the sketch.

## Hardware and software

- **Board:** Arduino UNO R4 WiFi (Renesas RA4M1, ARM Cortex-M4, 12x8 LED matrix, on-board LED on pin 13)
- **IDE:** Arduino IDE 2.x with the *Arduino UNO R4 Boards* core installed
- **Libraries:**
  - `Arduino_FreeRTOS` (Library Manager)
  - `Arduino_LED_Matrix` (bundled with the UNO R4 core)

## Concepts covered

**Already in the code**

- Creating tasks with `xTaskCreate` and starting the scheduler with `vTaskStartScheduler`
- Cooperative yielding (`taskYIELD`) versus blocking delays (`vTaskDelay`, `pdMS_TO_TICKS`)
- Integrating the Arduino `setup()` / `loop()` model into a FreeRTOS task
- Passing data to tasks through the `pvParameters` pointer
- Task priorities, and a dedicated task that owns the display
- Splitting firmware into headers and implementation files
- Wrapping RTOS setup inside a C++ class
- Driving the UNO R4 LED matrix with 96-bit frames stored as `uint32_t[3]`
- Setting single bits with `|=` and `1UL << n`

## Roadmap

The light patterns in the matrix projects are still **placeholders**: the **light games are to be defined**. They are meant to become the hands-on side of the theory of the following embedded topics, introduced one at a time:

- **Bit shifting** (`<<`, `>>`)
- **Bitwise logical operators** (AND, OR, XOR, NOT)
- **Masks** and bit fields, and the set / clear / toggle / test operations on a single bit
- Binary and hexadecimal notation, and mapping a (row, column) position onto the 96-bit frame
- Register-level thinking: configuring hardware by setting and clearing bits

The details of this plan are in the README of [`matrix_game_multi_task`](../../tree/matrix_game_multi_task). Further topics will be added to this list as they are covered.

## Author

Youssouf — [github.com/youssouf994](https://github.com/youssouf994)
