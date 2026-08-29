#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include <cstdint>
#include <cstdio>

#include "../hal/accelerometer.hpp"
#include "../hal/i2c.hpp"

// Routes std::printf output to the CCS Console over JTAG semihosting.
// Requires the debugger to stay connected and running - output is lost if
// you disconnect or power-cycle the board instead of debugging it live.
extern "C" void initialise_monitor_handles(void);

// TODO(lab4, step 2): Make this class inherit from and implement the
// `lab4::accelerometer` interface (`accelerometer.hpp`). Note that the
// BMI160 boots into a low-power suspend mode, so before its acceleration
// registers hold live data you must write a "set accelerometer to normal
// power mode" command to its command register, and you may also want to set
// its output data rate/range via its accelerometer configuration register.
// See the BMI160 datasheet's register map (command, power mode, and
// acceleration data registers).
class bmi160
{
  // Fill this out...
};

int main()
{
  // Stop the watchdog timer. Without this, the device resets a few seconds
  // after boot. DriverLib is allowed for this lab's I2C driver, but this
  // call is unrelated boilerplate that applies regardless.
  MAP_WDT_A_holdTimer();

  initialise_monitor_handles();

  std::printf("Hello, World\n");

  // TODO(lab4, step 3): Construct an i2c driver object and use it to
  // construct a bmi160 object.

  while (true) {
    // TODO(lab4, step 3): Read the X, Y, Z acceleration values from the
    // bmi160 object, compute roll and pitch, and print them to stdout. This
    // runs every loop iteration so roll and pitch are reported continuously.
  }
}
