#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include <cstdio>

#include "../hal/gpio.hpp"
#include "../hal/timer.hpp"

// Routes std::printf output to the CCS Console over JTAG semihosting.
// Requires the debugger to stay connected and running - output is lost if
// you disconnect or power-cycle the board instead of debugging it live.
extern "C" void initialise_monitor_handles(void);

int main()
{
  // Stop the watchdog timer. Without this, the device resets a few seconds
  // after boot. This is the only DriverLib call allowed in this lab - it is
  // boilerplate, not part of the drivers you are writing.
  MAP_WDT_A_holdTimer();

  initialise_monitor_handles();

  std::printf("Hello, World\n");

  // TODO(lab2, step 2): Configure the RGB LED pins as outputs and S1/S2 as
  // edge-triggered interrupt inputs (both in gpio.hpp), then a PWM output
  // and a time base (both in timer.hpp).

  // TODO(lab2, step 3): Wait for the first button press, then run the
  // rhythm game loop described in README.md: cycle the LED through the
  // color sequence at your chosen BPM, score each press by reaction
  // time, and end the game on a wrong press, a missed window, or a press
  // while the LED is off.

  while (true) {
    // TODO(lab2, step 3): game loop.
  }
}
