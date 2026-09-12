#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include <cstdio>

#include <cstdint>
#include <cstdlib>

#include "../hal/gpio.hpp"
#include "../hal/timer.hpp"
#include "../hal/timer_util.hpp"

// Routes std::printf output to the CCS Console over JTAG semihosting.
// Requires the debugger to stay connected and running - output is lost if
// you disconnect or power-cycle the board instead of debugging it live.
extern "C" void initialise_monitor_handles(void);

// Test double for lab2::steady_clock. Does not touch real hardware - it just
// increments a counter every time uptime() is called, as if a timer tick had
// elapsed. Useful for exercising delay logic before your real Timer_A-backed
// steady_clock exists.
class fake_steady_clock : public lab2::steady_clock
{
public:
  fake_steady_clock() = default;

private:
  std::uint32_t driver_frequency() override
  {
    // Use a small frequency to reduce the amount of ticks needed
    // for the delay function.
    return 100'000;
  }

  std::uint64_t driver_uptime() override
  {
    return m_count++;
  }

  std::uint64_t m_count = 0;
};

int main()
{
  using namespace std::chrono_literals;

  // Stop the watchdog timer. Without this, the device resets a few seconds
  // after boot. This is the only DriverLib call allowed in this lab - it is
  // boilerplate, not part of the drivers you are writing.
  MAP_WDT_A_holdTimer();

  initialise_monitor_handles();

  std::printf("Hello, World\n");

  // TODO(lab2, step 1): Implement lab2::steady_clock

  // TODO(lab2, step 2): Pass your steady clock to lab2::delay() from
  // hal/timer_util.hpp and test it with printf or blinking an LED - your
  // choice. Put a printf on either side of the delay and confirm the gap
  // between them matches the duration you asked for. If a 1s delay is not
  // taking 1 second, your frequency() is wrong.

  // TODO(lab2, step3): Implement lab2::pwm using what you learned from
  // lab2::steady_clock

  // TODO(lab2, step4): Test against an LED and see if you can control the
  // brightness
  fake_steady_clock clock;

  while (true) {
    lab2::delay(clock, 1s);
    std::printf("Sleep 1\n");
    lab2::delay(clock, 1s);
    std::printf("Sleep 2\n");
    lab2::delay(clock, 1s);
    std::printf("Sleep 3\n");
    // TODO(lab2, step 5): Use the steady clock together with your PWM driver to
    // animate the RGB LED as a continuous color wheel, as described in
    // README.md.
  }
}
