# Lab 1 - GPIO Driver

In this lab you will:

1. Install the IDE (Code Composer Studio).
2. Write a GPIO driver from scratch using the reference material - TI
   DriverLib is not allowed.
3. Demonstrate control over an RGB LED based on user input.

## Overview

You will build a minimal GPIO driver for the MSP432P401R directly against its
registers. Do not use TI DriverLib for the GPIO driver itself (the starter
code's single `MAP_WDT_A_holdTimer()` call to stop the watchdog is boilerplate
and does not count). Use the driver to read the two side push-buttons and drive
an RGB LED. This is the foundational bare-metal skill for the rest of the
course: given a datasheet and a Technical Reference Manual (TRM), you should be
able to find the memory-mapped registers that control a peripheral and drive
them correctly yourself, without depending on a vendor abstraction layer.

## Learning Objectives

- Import and build a Code Composer Studio (Theia) "nortos" project
  targeting the MSP-EXP432P401R LaunchPad, and flash it over the on-board
  XDS110 debug probe.
- Read the MSP432P4xx TRM well enough to locate the GPIO (`PxDIR`, `PxOUT`,
  `PxIN`, `PxREN`, `PxSEL0`/`PxSEL1`) register maps for a specific pin.
- Implement a concrete class against an provided abstract
  interface (pure virtual functions), using direct memory-mapped
  register access.
- Apply this repo's register-access conventions (`style.md` §S.10,
  Memory-Mapped I/O) to real hardware.

## Hardware

Board: MSP-EXP432P401R LaunchPad.

### RGB LED Pins

| Color | Pin  |
| ----- | ---- |
| Red   | P2.0 |
| Green | P2.1 |
| Blue  | P2.2 |

The two push-buttons (S1, S2) are also wired to GPIO pins, but you'll need to
find exactly which ones yourself from the
[MSP-EXP432P401R LaunchPad User's Guide](../datasheets/MSP%E2%80%91EXP432P401R_user_guide.pdf).

> [!TIP]
> Hint: consider which pull resistors you need for these switches, if any.

## Grading Rubric (50 pt)

1. **Toolchain setup - 10 pts.** Install CCS along with the **SimpleLink
   MSP432 SDK** (3.40.1.02) - this is what lets CCS resolve this project's
   `${COM_TI_SIMPLELINK_MSP432_SDK_*}` build variables. Then import the
   `cmpe146-2026-MSP-EXP432P401R` repo as a new workspace folder, build this
   project, and flash/debug it as-is. Minimum required work for points: the
   application prints `"Hello, World"` - the starter code already does this.
1. **Implement `output_pin` driver for MSP432P401R - 10 pts** - Write a
   concrete class that implements the `output_pin` interface.
1. **Implement `input_pin` driver for MSP432P401R - 10 pts** - Write a
   concrete class that implements the `input_pin` interface.
1. **Switch Sense & LED Control demonstration - 10 pts.** Using your gpio
   drivers:
   - Pressing S1 turns the RGB LED to a color (any color).
   - Pressing S2 turns the RGB LED to a different color than S1.
   - Pressing both switches turns the RGB to a 3rd color.
   - The instructor will request that one of the colors be changed
     during the demo and you are responsible for demonstrate that you can
     changing the color, rebuild, and reflash, and demonstrate that the new
     color is working.
1. **Code review - 10 pts.** Submit a PR to your git repo targeting your
   `main` branch.

## Reference material

- **[MSP432P4xx Family Technical Reference Manual](../datasheets/MSP432P4xx%20Family%20Technical%20Reference%20Manual.pdf)** -
  Digital I/O (GPIO) chapter. This is where the register offsets and bit
  fields you need actually live.
- **[MSP432P401R datasheet](../datasheets/datasheet_slas826e.pdf)** -
  pin-to-peripheral function table, electrical characteristics, peripheral
  memory map.
- **[MSP-EXP432P401R LaunchPad User's Guide](../datasheets/MSP%E2%80%91EXP432P401R_user_guide.pdf)** -
  on-board LED/button wiring and BoosterPack pinout.
- **[`style.md` §S.10 (Memory-Mapped I/O)](../style.md#s10-memory-mapped-io)** -
  the pattern this codebase uses for declaring and accessing hardware
  registers safely (`volatile` register-map structs, `reinterpret_cast` from
  a named `constexpr` base address, scoped to an anonymous namespace).
