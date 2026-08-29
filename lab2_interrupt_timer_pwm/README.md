# Lab 2 - Interrupt, Timers and PWM

In this lab you will:

1. Implement a steady clock, edge-triggered GPIO interrupt, and PWM
   driver from scratch - TI DriverLib is not allowed.
2. Debounce a switch interrupt in software.
3. Build a simplified rhythm game that coordinates all four drivers
   (including `gpio` from lab 1).

## Overview

Build a simplified
[Taiko no Tatsujin](https://www.youtube.com/watch?v=fAhLc4QtqvA)-style
rhythm game. The RGB LED cycles through RED, BLUE, and OFF at a fixed
beats-per-minute:

- RED - press S1
- BLUE - press S2
- OFF - press nothing

Wrong button, missed window, or a press during OFF ends the game. Score each
correct press by reaction time:

  1. Perfect
  2. Great
  3. Okay
  4. Bad

Print the final score when the game ends (loss or level complete).

BPM is your choice. Hardcode it or read it from STDIN. Encode the color
sequence however you like (array(s), generated) but try and keep it fair and
engaging. The lab instructor and TA will play the game to confirm it
works.

## Learning Objectives

- Implement a steady clock (time base) driver from scratch.
- Implement an edge-triggered GPIO interrupt driver from scratch.
- Implement a PWM driver from scratch.
- Debounce a switch interrupt signal in software.
- Coordinate multiple drivers (GPIO, clock, interrupt, PWM) to build a
  working game.

## Hardware

Board: MSP-EXP432P401R LaunchPad. Same LED wiring as `lab1_gpio`:

| Color | Pin  |
| ----- | ---- |
| Red   | P2.0 |
| Green | P2.1 |
| Blue  | P2.2 |

S1/S2 pins: see
[lab1_gpio/README.md](../lab1_gpio/README.md#hardware) or the
[MSP-EXP432P401R LaunchPad User's Guide](../datasheets/MSP%E2%80%91EXP432P401R_user_guide.pdf).

## Grading Rubric (50 pt)

1. **Steady clock - 10 pts.** Implemented and working.
2. **Edge-triggered interrupt - 10 pts.** Implemented and working.
3. **PWM - 10 pts.** Implemented and working.
4. **Switch debouncing - 10 pts.** Working debounce system.
5. **Reaction game - 10 pts.** Works as expected; the instructor and TA
   will play it to confirm.

## Reference material

- **[MSP432P4xx Family Technical Reference Manual](../datasheets/MSP432P4xx%20Family%20Technical%20Reference%20Manual.pdf)** -
  Digital I/O, Timer_A, and NVIC chapters - pin-to-peripheral function table
  (which pins carry which timer's PWM output) is in the datasheet below.
- **Armv7-M Architecture Reference Manual** - SysTick and Nested Vectored
  Interrupt Controller (NVIC), if you use them.
- **[MSP432P401R datasheet](../datasheets/datasheet_slas826e.pdf)** -
  pin-to-peripheral function table.
- **[MSP-EXP432P401R LaunchPad User's Guide](../datasheets/MSP%E2%80%91EXP432P401R_user_guide.pdf)** -
  on-board LED/button wiring.
- **[`style.md` §S.10 (Memory-Mapped I/O)](../style.md#s10-memory-mapped-io)** -
  the pattern this codebase uses for declaring and accessing hardware
  registers safely.

## HINT: DEBOUNCE

A closed switch vibrates, connecting and disconnecting the contact pad
multiple times before settling - each vibration fires the interrupt
again. Accept all of them and the game fails on the first press.
Disable the edge interrupt after the first pulse, then re-enable it on
the next LED color cycle.
