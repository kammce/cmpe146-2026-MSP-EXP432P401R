#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include <cstdint>
#include <cstdio>

#include "../hal/accelerometer.hpp"
#include "../hal/crc.hpp"
#include "../hal/gpio.hpp"
#include "../hal/i2c.hpp"
#include "../hal/serial.hpp"
#include "../hal/timer.hpp"

// Routes std::printf output to the CCS Console over JTAG semihosting.
// Requires the debugger to stay connected and running - output is lost if
// you disconnect or power-cycle the board instead of debugging it live.
extern "C" void initialise_monitor_handles(void);

// TODO(lab6, step 1): Make this class inherit from and implement the
// `lab6::serial` interface (`serial.hpp`), using DriverLib or register-level
// code. driver_receive_buffer()/driver_receive_cursor() must be backed by a
// circular buffer that a DMA transfer fills continuously in the background.
// See `lab6::serial::receive_cursor()`'s docs for how a caller is expected to
// consume it.
class serial_driver
{};

// TODO(lab6, step 2): Make this class inherit from and implement the
// `lab6::crc32` interface (`crc.hpp`), written register-level against the
// on-chip CRC accelerator peripheral - no DriverLib. This is the
// hardware-accelerated implementation you'll benchmark against the
// `crc32_software` reference supplied below.
class crc32_hardware
{};

// Supplied reference implementation - do not modify. This is the standard
// reflected CRC-32 (IEEE 802.3 / zlib) algorithm, computed bit-by-bit in
// software. Your crc32_hardware driver's output must match this for the
// same input, and should be measurably faster.
class crc32_software : public lab6::crc32
{
private:
  void driver_reset() override
  {
    m_crc = initial_value;
  }

  void driver_update(std::span<std::uint8_t const> p_data) override
  {
    for (auto const byte : p_data) {
      m_crc ^= byte;
      for (int bit = 0; bit < 8; bit++) {
        std::uint32_t const mask = -(m_crc & 1U);
        m_crc = (m_crc >> 1) ^ (polynomial & mask);
      }
    }
  }

  std::uint32_t driver_checksum() override
  {
    return m_crc ^ initial_value;
  }

  static constexpr std::uint32_t polynomial = 0xEDB8'8320U;
  static constexpr std::uint32_t initial_value = 0xFFFF'FFFFU;

  std::uint32_t m_crc = initial_value;
};

// TEMP: FreeRTOS task-switch smoke test. Two equal-priority tasks, each with
// its own statically-allocated stack + TCB (no heap involved). Each task
// delays instead of busy-looping forever, because configUSE_TIME_SLICING is
// 0 in FreeRTOSConfig.h - without a delay/yield, equal-priority tasks never
// hand off the CPU to each other. Delete this block once you start building
// the real lab6 flow (that work belongs inside a task body, not in main()
// after the scheduler starts - see the note below main()).
namespace {
constexpr std::uint32_t task_stack_words = 512;

StaticTask_t task_one_tcb;
StackType_t task_one_stack[task_stack_words];

StaticTask_t task_two_tcb;
StackType_t task_two_stack[task_stack_words];

void task_one(void*)
{
  std::uint32_t count = 0;
  while (true) {
    std::printf("task_one: %lu\n", static_cast<unsigned long>(count++));
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void task_two(void*)
{
  std::uint32_t count = 0;
  while (true) {
    std::printf("task_two: %lu\n", static_cast<unsigned long>(count++));
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
}  // namespace

int main()
{
  // Stop the watchdog timer. Without this, the device resets a few seconds
  // after boot. This is the only DriverLib call allowed for the CRC driver -
  // it is boilerplate, not part of the drivers you are writing.
  MAP_WDT_A_holdTimer();

  initialise_monitor_handles();

  std::printf("Hello, World\n");

  // TODO(lab6, step 3): Construct your accelerometer driver (lab4), a
  // serial_driver, and a crc32_hardware (crc32_software above needs no setup
  // - it's ready to use as-is). Construct a lab1::input_pin for the
  // start/stop button and a lab2::steady_clock for timing the CRC benchmark.

  // NOTE(lab6, step 4): Once you're past the smoke test below, this is where
  // that logic goes - but it belongs inside a task body, not here. main()
  // never returns from vTaskStartScheduler(), so anything after it only
  // runs if scheduler startup itself fails. Wait for the button to signal
  // start. Once started, the two devices take turns: gather the IMU
  // reading, compute its CRC with both your crc32_hardware and the supplied
  // crc32_software (timing each with the steady_clock and printing both
  // durations - crc32_hardware should win), transmit the data with the CRC
  // appended, then wait to receive the other device's data and validate its
  // CRC and print its results. Repeat until the button signals stop.

  xTaskCreateStatic(task_one,
                    "task_one",
                    task_stack_words,
                    nullptr,
                    tskIDLE_PRIORITY + 1,
                    task_one_stack,
                    &task_one_tcb);
  xTaskCreateStatic(task_two,
                    "task_two",
                    task_stack_words,
                    nullptr,
                    tskIDLE_PRIORITY + 1,
                    task_two_stack,
                    &task_two_tcb);

  vTaskStartScheduler();

  // Only reached if vTaskStartScheduler() itself fails to start (e.g. the
  // idle/timer task memory callbacks below returned something invalid).
  while (true) {
    continue;
  }
}

// FreeRTOS calls these to get memory for the idle and timer service tasks
// now that configSUPPORT_STATIC_ALLOCATION is 1 - required any time both
// static and dynamic allocation are enabled together, regardless of whether
// your own tasks use static or dynamic creation.
extern "C" void vApplicationGetIdleTaskMemory(
  StaticTask_t** ppxIdleTaskTCBBuffer,
  StackType_t** ppxIdleTaskStackBuffer,
  uint32_t* pulIdleTaskStackSize)
{
  static StaticTask_t idle_task_tcb;
  static StackType_t idle_task_stack[configMINIMAL_STACK_SIZE];

  *ppxIdleTaskTCBBuffer = &idle_task_tcb;
  *ppxIdleTaskStackBuffer = idle_task_stack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

extern "C" void vApplicationGetTimerTaskMemory(
  StaticTask_t** ppxTimerTaskTCBBuffer,
  StackType_t** ppxTimerTaskStackBuffer,
  uint32_t* pulTimerTaskStackSize)
{
  static StaticTask_t timer_task_tcb;
  static StackType_t timer_task_stack[configTIMER_TASK_STACK_DEPTH];

  *ppxTimerTaskTCBBuffer = &timer_task_tcb;
  *ppxTimerTaskStackBuffer = timer_task_stack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

// Required because FreeRTOSConfig.h sets configUSE_MALLOC_FAILED_HOOK and
// configCHECK_FOR_STACK_OVERFLOW - the kernel calls these unconditionally
// once anything in tasks.c/heap_4.c that references them gets linked in.
extern "C" void vApplicationMallocFailedHook(void)
{
  while (true) {
    continue;
  }
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char*)
{
  while (true) {
    continue;
  }
}
