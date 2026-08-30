#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include <array>
#include <cstdint>
#include <cstdio>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "../hal/adc.hpp"
#include "../hal/crc.hpp"
#include "../hal/gpio.hpp"
#include "../hal/serial.hpp"
#include "../hal/timer.hpp"

// Routes std::printf output to the CCS Console over JTAG semihosting.
// Requires the debugger to stay connected and running - output is lost if
// you disconnect or power-cycle the board instead of debugging it live.
extern "C" void initialise_monitor_handles(void);

// TODO(lab, step 1): Make this class inherit from and implement the
// `lab5::adc16_stream` interface (`adc.hpp`), wired to the TMP36's ADC
// channel. DriverLib is allowed for this driver. read() must configure a
// single DMA transfer that fills the entire given buffer with consecutive
// conversions, put the calling FreeRTOS task to sleep (e.g. block on a
// semaphore or task notification) for the duration of that transfer, and
// let the DMA transfer-complete interrupt wake it once the buffer is full -
// one sleep per buffer, not one per sample.
class adc_driver
{};

// TODO(lab, step 2): Make this class inherit from and implement the
// `lab6::serial` interface (`serial.hpp`), using DriverLib or register-level
// code. driver_receive_buffer()/driver_receive_cursor() must be backed by a
// circular buffer that a second, independent DMA transfer fills continuously
// in the background - this is a separate DMA channel from the one the ADC
// driver uses above. See `lab6::serial::receive_cursor()`'s docs for how a
// caller is expected to consume it.
class serial_driver
{};

// TODO(lab, step 3): Make this class inherit from and implement the
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

// TMP36 outputs 500 mV at 0 degC and scales linearly at 10 mV/degC (see the
// TMP36 datasheet's "Temperature Conversion" section). p_sample follows
// `lab5::adc16_stream::read()`'s contract: a 16-bit value proportional to
// the measured voltage from Vss (0V) to Vcc.
//
// TODO(lab, step 4): Confirm the Vcc you actually wired the TMP36 and ADC
// reference to - this assumes 3.3V. If your reference voltage differs,
// update tmp36_vcc_millivolts to match, or the conversion will be wrong.
constexpr float tmp36_vcc_millivolts = 3300.0F;

float tmp36_to_celsius(std::uint16_t p_sample)
{
  float const millivolts =
    (static_cast<float>(p_sample) / 65535.0F) * tmp36_vcc_millivolts;
  return (millivolts - 500.0F) / 10.0F;
}

namespace {
constexpr std::size_t samples_per_average = 64;
constexpr std::size_t queue_size = 1;
constexpr std::uint32_t task_stack_words = 512;

StaticQueue_t temperature_queue_control_block;
std::array<float, queue_size> temperature_queue_storage;
QueueHandle_t temperature_queue;

StaticTask_t adc_task_tcb;
StackType_t adc_task_stack[task_stack_words];

StaticTask_t exchange_task_tcb;
StackType_t exchange_task_stack[task_stack_words];

void adc_task(void*)
{
  // TODO(lab, step 5): Construct an adc_driver. Every iteration, declare a
  // local `std::array<std::uint16_t, samples_per_average>` buffer, fill it
  // with one call to adc_driver::read() (one DMA transfer, one task sleep),
  // average the buffer's contents, convert the average to Celsius with
  // tmp36_to_celsius(), and send the result onto temperature_queue for the
  // exchange task to pick up.
  while (true) {
    continue;
  }
}

void exchange_task(void*)
{
  // TODO(lab, step 6): Construct a serial_driver, a crc32_hardware
  // (crc32_software above needs no setup - it's ready to use as-is), a
  // lab1::input_pin for the start/stop button, and a lab2::steady_clock for
  // timing the CRC benchmark. Wait for the button to signal start. Once
  // started, on each pass: block on temperature_queue for the next averaged
  // reading, compute its CRC-32 with both crc32_hardware and
  // crc32_software (timing each with the steady_clock and printing both
  // durations - crc32_hardware should win), transmit the reading with the
  // CRC appended over UART, then wait to receive the other board's packet,
  // validate its CRC, and print its result. Repeat until the button
  // signals stop.
  while (true) {
    continue;
  }
}
}  // namespace

int main()
{
  // Stop the watchdog timer. Without this, the device resets a few seconds
  // after boot. This is the only DriverLib call allowed outside of the ADC
  // driver - it is boilerplate, not part of the drivers you are writing.
  MAP_WDT_A_holdTimer();

  initialise_monitor_handles();

  std::printf("Hello, World\n");

  temperature_queue = xQueueCreateStatic(
    temperature_queue_storage.size(),
    sizeof(temperature_queue_storage[0]),
    reinterpret_cast<std::uint8_t*>(temperature_queue_storage.data()),
    &temperature_queue_control_block);

  xTaskCreateStatic(adc_task,
                    "adc_task",
                    task_stack_words,
                    nullptr,
                    tskIDLE_PRIORITY + 1,
                    adc_task_stack,
                    &adc_task_tcb);
  xTaskCreateStatic(exchange_task,
                    "exchange_task",
                    task_stack_words,
                    nullptr,
                    tskIDLE_PRIORITY + 1,
                    exchange_task_stack,
                    &exchange_task_tcb);

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
