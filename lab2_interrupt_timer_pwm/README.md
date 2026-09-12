# Lab 2 - Timers and PWM

In this lab you will:

1. Implement a steady clock and a PWM driver from scratch - TI DriverLib is
   not allowed.
2. Use the steady clock to build your own blocking delay function.
3. Use the PWM driver to animate a continuous RGB "color wheel" on the
   on-board LED.

## Overview

Drive the on-board RGB LED as a smooth, continuously animating color wheel:
each of the red, green, and blue channels gets its own PWM signal, and their
duty cycles rise and fall out of phase with each other (for example, 120
degrees apart) so the LED sweeps through colors forever. The whole lab is
getting a real time base and real PWM working, then combining them.

- **Timer -> delay.** Implement `lab2::steady_clock` (declared in
  `hal/timer.hpp`) on top of a Timer_A peripheral. Once its `uptime()` and
  `frequency()` work, the provided `lab2::delay()` in `hal/timer_util.hpp`
  gives you a blocking delay for free. This is what paces your animation
  loop.
- **PWM -> brightness.** Implement `lab2::pwm` (also in `hal/timer.hpp`) on
  top of a Timer_A compare/capture channel. Use `duty_cycle()` to control
  how bright each LED channel is.
- **Combine.** In `main()`, continuously update the three PWM duty cycles -
  from a lookup table, a computed waveform, or whatever scheme you like -
  and call your delay function between updates so the animation runs at a
  visible, steady rate.

Encode the color sequence however you like (a lookup table, a formula,
three phase-shifted counters) - just make it visibly smooth and continuous.

## Learning Objectives

- Implement a steady clock (time base) driver from scratch.
- Implement a PWM driver from scratch.
- Convert a desired frequency into the correct timer register values
  (period and compare), instead of hardcoding them.
- Use a hardware timer to build a blocking delay function.
- Drive multiple PWM channels out of phase to animate a color transition.

## Hardware

Board: MSP-EXP432P401R LaunchPad. Same LED wiring as `lab1_gpio`:

| Color | Pin  |
| ----- | ---- |
| Red   | P2.0 |
| Green | P2.1 |
| Blue  | P2.2 |

## Provided code

| File                 | What it gives you                                                                     |
| -------------------- | ------------------------------------------------------------------------------------- |
| `hal/timer.hpp`      | The `lab2::steady_clock` and `lab2::pwm` interfaces you implement.                    |
| `hal/timer_util.hpp` | `lab2::delay()` and friends, built on top of whatever `lab2::steady_clock` you write. |

You do not need to write a delay function - `hal/timer_util.hpp` already has
one. It only needs a working `lab2::steady_clock`, so as soon as your
`uptime()` and `frequency()` are correct, this works:

```cpp
#include "../hal/timer_util.hpp"

using namespace std::chrono_literals;

my_steady_clock clock;

lab2::delay(clock, 500ms);   // blocks for half a second
lab2::delay(clock, 20us);    // blocks for 20 microseconds
```

It is worth reading that header rather than just calling it. `delay()` is only
about five lines; the interesting part is `duration_to_ticks()`, which converts
a real duration into a tick count for your specific clock. The comments explain
why the multiply is split into whole seconds plus a remainder instead of being
done all at once - the same overflow concern that shows up in the duty cycle
math below.

This also means `lab2::delay()` is a good way to sanity check your steady
clock: if `lab2::delay(clock, 1s)` does not actually take about a second, your
`frequency()` is reporting the wrong number.

## Requirements

1. **Steady clock.** Implement `lab2::steady_clock` using a Timer_A
   peripheral. `frequency()` must return the timer's actual operating
   frequency, not a placeholder. You may pick whatever operating frequency
   you like, just make sure the driver reports it correctly.
2. **PWM.** Implement `lab2::pwm` using a Timer_A compare/capture channel.
   Give your PWM class a way to configure its frequency via a constructor
   parameter and use that frequency to compute the period (`TAxCCR0`) register
   value. Default to the timer's out-of-reset clock source: **SMCLK, sourced
   from the DCO, running at 3 MHz**. Unless you have a specific reason to
   reconfigure the clock system, that 3 MHz is the input to all of your
   frequency math.
3. **Delay.** Use `lab2::delay()` from `hal/timer_util.hpp` with your steady
   clock to block for a specified duration. Verify it against a clock or a
   stopwatch. One second delay should take one second. If it does not, the
   bug is in your steady clock, not in `delay()`.
4. **Color wheel.** Combine the two drivers above to continuously animate
   the RGB LED as described in Overview.

## Grading Rubric (50 pt)

1. **Steady clock - 15 pts.** Implemented, running, and reports the correct
   frequency.
2. **PWM - 20 pts.** Implemented, running, frequency is configurable at
   construction (not hardcoded), and is correctly translated into register values.
3. **Color wheel animation - 15 pts.** Smooth, continuous, and visible on
   hardware; I will observe it running.

## Reference material

- **[MSP432P4xx Family Technical Reference Manual](../datasheets/MSP432P4xx%20Family%20Technical%20Reference%20Manual.pdf)** -
  Digital I/O, Timer_A, and Clock System (CS) chapters - the pin-to-peripheral
  function table (which pins carry which timer's PWM output) is in the
  datasheet below.
- **[MSP432P401R datasheet](../datasheets/datasheet_slas826e.pdf)** -
  pin-to-peripheral function table.
- **[MSP-EXP432P401R LaunchPad User's Guide](../datasheets/MSP%E2%80%91EXP432P401R_user_guide.pdf)** -
  on-board LED wiring.

## HINT: FREQUENCY MATH

The PWM and steady clock drivers share the same timer hardware, so the
frequency math has the same shape for both:

1. Use SMCLK (Subsystem Master Clock, sourced from the DCO out of reset -
   default 3MHz) as the input clock source for the timer
2. Pick a divider if you want a different tick rate than the raw input
   clock.
3. For the steady clock, that resulting tick rate is exactly what
   `frequency()` must return.
4. For PWM in Up mode, `CCR0` sets how many ticks make up one waveform
   cycle - which sets the frequency. A `CCRn` (n = 1, 2, 3, 4) compare
   register sets how many of those ticks are HIGH - which sets the duty
   cycle. `CCR0` comes from your frequency math in the steps above; don't
   hardcode it.

   Duty cycle comes in as a 16-bit fraction of full scale: `0` is 0% and
   `65535` (`0xFFFF`, or 2^16 - 1) is 100%. Half brightness is `32767`, and
   so on. So `CCRn` is just `CCR0` scaled by that fraction:

   ```cpp
   // Assume CCR0 and CCRn are stand-ins for the actual registers
   auto const ccr0 = static_cast<std::uint32_t>(*CCR0);
   *CCRn = static_cast<std::uint16_t>((ccr0 * p_duty_cycle) / 65535UL);
   ```

   > [!NOTE]
   > The `static_cast<std::uint32_t>` on `CCR0` is load-bearing, and it has
   > to be on an operand of the *multiply*. Two `std::uint16_t` values
   > promote to `int` (not `unsigned`) before multiplying, and
   > 65535 x 65535 = 4,294,836,225 overflows a 32-bit signed `int`, which
   > is undefined behavior, not a wrapped value. Widening the divisor
   > instead would be too late, since the division happens after the
   > product has already been computed. With the cast, the multiply is done
   > in unsigned 32-bit math, where the worst case still fits, and the
   > result is narrowed back to `std::uint16_t` only at the very end.

> [!NOTE]
> If you want to verify the 3 MHz for yourself, it comes from two places in
> the Clock System (CS) chapter of the TRM: section 5.2 (p. 294) states that
> after reset "DCOCLK is selected for MCLK, HSMCLK, and SMCLK", and the
> `CSCTL0` register description (p. 307) shows `DCORSEL` has a reset value
> of `1h`, which selects the 3 MHz nominal DCO range. The datasheet's Table
> 5-11 (p. 51) confirms the measured center frequency for that range. Page 14
> of the user guide also states that the default for the CPU and peripherals
> is the DCO at 3 MHz.

## HINT: Registers you can ignore

The registers below can be safely ignored for this lab.

```plaintext
2Eh TAxIV Timer_Ax Interrupt Vector Section 17.3.5
20h TAxEX0 Timer_Ax Expansion 0 Section 17.3.6
```
