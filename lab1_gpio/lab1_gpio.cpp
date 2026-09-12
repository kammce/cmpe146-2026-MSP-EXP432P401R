#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include <cstdio>

#include "../hal/gpio.hpp"

// Routes std::printf output to the CCS Console over JTAG semihosting.
// Requires the debugger to stay connected and running - output is lost if
// you disconnect or power-cycle the board instead of debugging it live.
extern "C" void initialise_monitor_handles(void);

class msp432_input : public lab1::input_pin
{
  msp432_input(int port, int pin)
  {
    if (port & 1) {
      mask = 1 << pin;
    } else {
      mask = 1 << (pin + 8);
    }
  }

  uint16_t mask;

  bool driver_configure(settings const& p_settings) override
  {
    return false;
  }
  bool driver_level() override
  {
    return false;
  }
};

int main()
{
  // Stop the watchdog timer. Without this, the device resets a few seconds
  // after boot. This is the only DriverLib call allowed in this lab - it is
  // boilerplate, not part of the GPIO driver you are writing.
  MAP_WDT_A_holdTimer();

  initialise_monitor_handles();

  std::printf("Hello, World\n");

  // TODO(lab1, step 1): Implement the `lab1::output_pin` interface
  // (../hal/gpio.hpp) for the MSP432P401R, register-level - no DriverLib.
  // Use it to drive the on-board RGB LED. See README.md for which port/pin
  // each LED color is wired to.

  // TODO(lab1, step 2): Implement the `lab1::input_pin` interface
  // (../hal/gpio.hpp) for the MSP432P401R, register-level - no DriverLib.
  // Use it to read the two push-buttons (S1, S2). See README.md for which
  // port/pin each button is wired to.

  while (true) {
    // TODO(lab1, step 3): implement the button -> LED color behavior
    // described in README.md
  }
}
