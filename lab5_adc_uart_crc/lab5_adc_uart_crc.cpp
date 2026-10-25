#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

#include <FreeRTOS.h>
#include <task.h>

#include "../hal/adc.hpp"
#include "../hal/crc.hpp"
#include "../hal/serial.hpp"
#include "../hal/timer.hpp"

// GNU rdimon provides printf output in the CCS CIO Console. The debugger
// check prevents semihosting calls from stopping a board running on its own.
extern "C" void initialise_monitor_handles(void);

namespace {
bool semihosting_ready = false;
std::array<char, 256> semihosting_stdout_buffer{};

bool debugger_connected()
{
  return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0;
}

template<typename... Args>
void debug_printf(char const* format, Args... args)
{
  if (semihosting_ready && debugger_connected()) {
    std::printf(format, args...);
  }
}

enum class device_role : std::uint8_t
{
  sensor,
  display,
};

// Change this before building each board.
constexpr device_role this_device = device_role::sensor;

// You may implement the Lab 5 drivers here or move each one into a driver
// header. Each constructor must leave its peripheral ready to use.
class msp432_adc_driver : public lab5::adc16_stream
{
  // TODO: Implement the ADC14 and DMA driver.
};

// eUSCI_A2 RX: P3.2 / J1.3
// eUSCI_A2 TX: P3.3 / J1.4
class msp432_serial_driver : public lab6::serial
{
  // TODO: Implement the UART driver and its circular receive buffer.
};

class msp432_crc32_hardware : public lab6::crc32
{
  // TODO: Implement the MSP432 CRC32 peripheral driver.
};

// Supplied software CRC-32 reference. Do not modify it. Use it for packet
// CRCs and compare its benchmark result with msp432_crc32_hardware.
class crc32_software : public lab6::crc32
{
private:
  void driver_reset() override
  {
    m_crc = initial_value;
  }

  void driver_update(std::span<std::uint8_t const> data) override
  {
    for (auto const byte : data) {
      m_crc ^= byte;
      for (int bit = 0; bit < 8; ++bit) {
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

// TODO: Add the request_task, sampler_task, transmit_task, and display_task
// described in README_MSP432.md.

void blinker_task(void*)
{
  // TODO: Replace the code below with one of your output pin implementations
  // msp432_output red_led(1, 0);  // LaunchPad LED1 on P1.0.

  while (true) {
    // red_led.level(not red_led.level());
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

constexpr std::uint32_t blinker_stack_words = 256;
StaticTask_t blinker_tcb;
StackType_t blinker_stack[blinker_stack_words];
}  // namespace

int main()
{
  WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

  if (debugger_connected()) {
    initialise_monitor_handles();
    std::setvbuf(stdout,
                 semihosting_stdout_buffer.data(),
                 _IOLBF,
                 semihosting_stdout_buffer.size());
    semihosting_ready = true;
  }

  // This task is complete. LED1 blinking shows that FreeRTOS is scheduling.
  configASSERT(xTaskCreateStatic(blinker_task,
                                 "blinker",
                                 blinker_stack_words,
                                 nullptr,
                                 tskIDLE_PRIORITY + 1,
                                 blinker_stack,
                                 &blinker_tcb) != nullptr);

  if constexpr (this_device == device_role::sensor) {
    debug_printf("Lab 5 sensor starter running.\n");
    // TODO: Create the sensor board's tasks and synchronization objects.
  } else {
    debug_printf("Lab 5 display starter running.\n");
    // TODO: Create the display task.
  }

  vTaskStartScheduler();

  configASSERT(false);
  while (true) {
  }
}

// FreeRTOS uses these buffers for its idle and timer service tasks.
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

extern "C" void vApplicationMallocFailedHook(void)
{
  while (true) {
  }
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t, char*)
{
  while (true) {
  }
}
