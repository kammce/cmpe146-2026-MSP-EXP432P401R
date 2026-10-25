# Lab 5 - ADC + UART + CRC (FreeRTOS)

For this lab, you will:

1. Implement `msp432_adc_driver` with DriverLib and DMA to sample an external
   TMP36 temperature sensor.
2. Implement register-level `msp432_serial_driver` with an interrupt-driven
   circular receive buffer.
3. Implement register-level `msp432_crc32_hardware` for the on-chip CRC-32
   accelerator and benchmark it against a supplied software CRC-32.
4. Convert raw ADC samples into temperatures.
5. Use FreeRTOS to build a request/response link between two boards: a
   **sensor** board and a **display** board.

## Use cases

- A central controller polling remote sensor nodes on demand
- Monitoring systems where the display is separate from the sensing hardware
- Any serial link where corrupted data must be caught before it is used

## Overview

Both roles live in `lab5_adc_uart_crc.cpp`. Set `this_device` near the top
of the file to pick the role, then rebuild and flash. The `if constexpr` in
`main()` compiles in only that role's code.

The exchange:

1. **Display:** wait for S1 or S2, then send `sample_request_command` (one
   byte) to the sensor board.
2. **Sensor:** take 64 temperature samples in one DMA transfer, convert
   them, append a CRC-32, and send the packet back. If the sensor board's S1
   is held while sending, flip one bit in the packet after computing the CRC.
3. **Display:** check the CRC. If it matches, set the RGB LED from the
   readings: blue for cold, yellow for warm, red for hot (you pick the
   bands). If it does not, drop the packet, print a message, and turn the LED
   magenta.

**Packet format:** 64 temperature samples followed by a 4-byte CRC. The
sample encoding is your choice, as long as it resolves at least 0.5 degC (a
whole-degree integer cannot). Agree on the encoding and byte order as a team
on day one, since both boards depend on it.

## Rules

- **DriverLib is required for `msp432_adc_driver`**, including its DMA setup.
  `msp432_serial_driver` and `msp432_crc32_hardware` are register-level.
- **All DriverLib calls go inside your driver classes.** Do not configure
  peripherals in SysConfig.
- **Each driver configures its peripheral completely in its constructor.**
  Do not add a separate `initialize()` or `configure()` phase.
- Create the FreeRTOS tasks directly with `xTaskCreateStatic()`.
- Do not use the MSP432's internal die-temperature sensor. Read the external
  TMP36 through P6.1 instead.

## FreeRTOS tasks

**Sensor board: three tasks**

- `request_task`: watches the UART for `sample_request_command`, then gives
  the `sample_request` semaphore.
- `sampler_task`: waits on `sample_request`, reads 64 ADC samples, converts
  them, and sends them to `batch_queue`.
- `transmit_task`: waits on `batch_queue`, computes both CRCs, and sends the
  packet.

**Display board: one tasks**

- `display_task`: sends the request on S1/S2, waits for the packet,
  validates the CRC, and sets the LED.

**Blinker Task**

The `blinker_task` toggles the separate red LED on P1.0 every 250 ms so you
can see that FreeRTOS continues scheduling while `display_task` waits. Not
necessary but helpful to get a visual of whether or not the whole system is
locked up.

**How the sensor tasks pass data**

```text
UART RX
   |
request_task
   |   sample_request (binary semaphore): "take a batch now"
   v
sampler_task   <-- DMA_INT1_IRQHandler (task notification): "buffer is full"
   |   batch_queue (queue): a copy of the 64 converted samples
   v
transmit_task
   |
UART TX
```

- `sample_request` carries no data, only the signal to start.
- `batch_queue` carries the data. Each item is a full copy of the 64
  samples, so the two tasks never share a buffer.
- The task notification lives inside `adc.read()`.

## Team roles

Each person owns one role and defends it at check-off. List the owners in
your PR description. Open your role's guide below; you only need to read
your own.

<details>
<summary><b>1. ADC: <code>msp432_adc_driver</code>, temperature conversion, <code>sampler_task</code></b></summary>

Start with the MSP432 SDK example
`examples/nortos/MSP_EXP432P401R/driverlib/adc14_single_conversion_repeat`.
The ADC14, DMA, and REF_A chapters of the technical reference manual contain
the rest of the configuration you need.

**Constructor**

1. Configure P6.1 for its ADC14 A14 function.
2. Enable ADC14 and select its clock source and divider.
3. Configure unsigned, 14-bit, repeated single-channel conversions.
4. Configure ADC14 memory 0 to read A14 against the internal 2.5 V
   reference.
5. Select a sample-and-hold time that satisfies the ADC14 and sensor timing
   requirements.
6. Configure REF_A for 2.5 V and wait for the reference to become ready.
7. Configure DMA channel 7 for the ADC14 trigger: half-word transfers, fixed
   source address at ADC14 memory 0, and an incrementing destination.
8. Enable the DMA completion interrupt. ADC14 does not need a per-sample
   interrupt.

**Each `read()`**

1. Stop any previous conversion, disable the DMA channel, and wait until
   ADC14 is idle.
2. Point the DMA destination at the supplied buffer and set the transfer
   size to that buffer's size.
3. Save the calling task's handle from `xTaskGetCurrentTaskHandle()` where
   the interrupt handler can reach it. Do this before starting the transfer.
4. Re-enable the DMA channel and start the ADC conversion.
5. Sleep with `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)`.
6. Once woken, stop the conversion and upscale each 14-bit sample to 16 bits
   as `adc.hpp` requires.

**Interrupt handler**

1. Define `extern "C" void DMA_INT1_IRQHandler()`.
2. Confirm that DMA channel 7 completed and clear its interrupt flag.
3. Declare `BaseType_t higher_priority_task_woken = pdFALSE;` and call
   `vTaskNotifyGiveFromISR(saved_handle, &higher_priority_task_woken)`.
4. End with `portYIELD_FROM_ISR(higher_priority_task_woken)` so the woken
   task can run as soon as the handler returns.

**Temperature conversion**

- ADC14 uses a 2.5 V reference and a 14-bit result, so first convert the ADC
  code into the voltage at TMP36 VOUT.
- The TMP36 output is 500 mV at 0 degC and changes by 10 mV/degC. Use its
  datasheet transfer function to calculate temperature.

**`sampler_task`**

1. Block in `xSemaphoreTake(sample_request, portMAX_DELAY)`.
2. Call `adc.read()` on a `std::array<std::uint16_t, 64>`.
3. Convert each sample to `temperature_sample_t`.
4. `xQueueSend(batch_queue, &samples, portMAX_DELAY)` copies the converted
   array into the queue.

</details>

<details>
<summary><b>2. UART: <code>msp432_serial_driver</code></b></summary>

- Implement `msp432_serial_driver` with eUSCI_A2 then configure it to 9600
  baud, 8 data bits, no parity, and one stop bit.
- Register-level. The constructor receives the clock and serial settings,
  computes the baud-rate dividers, configures P3.2/P3.3 and eUSCI_A2, and
  enables the receive interrupt.
- Get polled transmit and receive working first so your team can test
  between boards early. Then move receive to the interrupt.
- On receive, the RX interrupt copies each byte into a circular buffer, and
  `receive_cursor()` returns where the next byte will go. See its docs in
  `serial.hpp`.
- Make the buffer larger than one packet. A full lap around the buffer looks
  like zero new bytes. 256 bytes is a safe size.
- The handler is `extern "C" void EUSCIA2_IRQHandler()`.
- Test alone by wiring J1.4 (TX) to J1.3 (RX) on one board.

</details>

<details>
<summary><b>3. CRC: <code>msp432_crc32_hardware</code>, benchmark, <code>transmit_task</code></b></summary>

**Driver**

- Register-level, against the on-chip CRC32 accelerator.
- Output must match `crc32_software` (standard CRC-32, as used by zlib).
  Check the accelerator's bit-order options and whether it applies the final
  XOR.
- The hardware implementation is used for the benchmark. Packet creation
  and validation use the supplied software CRC implementation.

**`transmit_task`**

1. Block in `xQueueReceive(batch_queue, &samples, portMAX_DELAY)`.
2. Compute the CRC over the samples with `msp432_crc32_hardware` and
   `crc32_software`, timing each with `lab2::steady_clock`. Print both
   durations and confirm that the results match.
3. Save the software CRC for the packet, then read the corruption button.
   If it is held, flip one bit in the samples.
4. Write the samples, then the saved CRC.

</details>

<details>
<summary><b>4. Application: <code>request_task</code>, <code>display_task</code>, <code>blinker_task</code>, semaphore, queue, <code>main()</code></b></summary>

**Setup in `main()`**

- Include `semphr.h` and `queue.h`.
- Before `vTaskStartScheduler()`, create `sample_request` with
  `xSemaphoreCreateBinaryStatic()`, `batch_queue` with
  `xQueueCreateStatic()` (a `StaticQueue_t` plus storage for one
  `std::array<temperature_sample_t, 64>`), and the three sensor tasks with
  `xTaskCreateStatic()`, each with its own stack and TCB.
- Construct every driver with the settings it needs. Do not call a second
  initialization function after construction.
- Give drivers shared with tasks static storage duration. Do not leave them
  on `main()`'s stack and retain pointers to them after the scheduler starts.
- The serial driver is shared: `request_task` only reads from it, and
  `transmit_task` only writes to it.
- On the display build, create both `display_task` and `blinker_task` with
  `xTaskCreateStatic()`. Give each task its own stack and TCB.

**`request_task`**

- Every few ms (`vTaskDelay()`), compare `receive_cursor()` with the last
  position you read. When `sample_request_command` arrives, call
  `xSemaphoreGive(sample_request)`.

**`display_task`**

1. On S1 or S2, save the current `receive_cursor()`, then send
   `sample_request_command`.
2. Wait until the full packet (64 samples plus the 4-byte CRC) has arrived
   after the saved position. If it takes more than 500 ms, print a timeout
   message.
3. Recompute the CRC over the samples with `crc32_software` and compare it
   to the one received. On a mismatch, print a message and set the LED
   magenta. Otherwise, average the samples and set the LED color.

**`blinker_task`**

- Construct an output for P1.0, toggle it, and call `vTaskDelay()` between
  changes. The task must sleep between toggles so other tasks can run.

</details>

Teams of 5 split role 4: one person takes `request_task`, the semaphore, and
the queue, and the other takes `display_task`.

Until a teammate's part works, use a stand-in behind the same interface: an
`adc16_stream` that returns a ramp (each sample one higher than the last),
`crc32_software` for the CRC, or a `serial` that prints whatever it is asked
to write.

**Goal for the end of week 1:** `msp432_crc32_hardware` matches
`crc32_software`, and the two boards can exchange bytes over UART.

## Hardware

Use two MSP-EXP432P401R LaunchPads, one per role.

**UART connection**

| Sensor board              | Display board             |
| ------------------------- | ------------------------- |
| P3.3 / J1.4 (eUSCI_A2 TX) | P3.2 / J1.3 (eUSCI_A2 RX) |
| P3.2 / J1.3 (eUSCI_A2 RX) | P3.3 / J1.4 (eUSCI_A2 TX) |
| GND                       | GND                       |

Power both LaunchPads before connecting the UART signals. An unpowered board
can be back-powered through its RX pin. Do not connect the boards' 3.3 V rails;
connect only the two crossed UART signals and ground. A 4.7 kOhm series
resistor in each UART signal limits current if one board loses power.

## Printf logging

Diagnostic `printf()` output uses GNU rdimon semihosting and appears in the
CCS CIO Console when a debugger is attached. Without a debugger, the firmware
will halt when `printf()` is called. Instead, please use `debug_printf` which
checks if the debugger is connected. The output is line buffered, so end each
diagnostic message with `\n`.

**TMP36 connection on the sensor board**

| TMP36 | LaunchPad                |
| ----- | ------------------------ |
| +VS   | 3.3 V / J1.1             |
| VOUT  | P6.1 / ADC14 A14 / J3.23 |
| GND   | J3.22 or J2.20           |

Check your TMP36 package's pinout before applying power. See
[lab1](../lab1_gpio/README.md#hardware) for the RGB LED and S1/S2 pins.
The display board also uses its separate red LED on P1.0 as the FreeRTOS
activity indicator.

## Grading Rubric (120 pt)

The whole team shares one grade. Roles decide who answers which questions at
check-off, not who earns which points.

### Role 1: ADC (25 pts)

- **ADC + DMA - 15 pts.** `msp432_adc_driver` fills the whole buffer in one DMA
  transfer and sleeps the calling task until the DMA interrupt wakes it.
- **Temperature conversion - 10 pts.** TMP36 samples convert to temperatures
  with at least 0.5 degC resolution.

### Role 2: UART (25 pts)

- **UART transmit - 10 pts.** Transmits at the configured baud rate.
- **UART receive - 15 pts.** Receives into an interrupt-driven circular
  buffer, and `receive_cursor()` behaves as `serial.hpp` describes.

### Role 3: CRC (20 pts)

- **CRC driver - 15 pts.** `msp432_crc32_hardware` matches `crc32_software`, and
  both implementations are benchmarked.
- **Corrupted data demonstration - 5 pts.** Holding the sensor board's S1
  flips a bit, and the display board catches it.

### Role 4: Application (25 pts)

- **Display role - 15 pts.** S1/S2 requests a batch and the LED shows the
  temperature. Bad CRCs turn the LED magenta, and both bad CRCs and timeouts
  print a message. The separate P1.0 red LED continues blinking.
- **Sensor task architecture - 10 pts.** Three statically allocated tasks,
  the `sample_request` semaphore, and `batch_queue`, as described above.

### Whole team (25 pts)

- **End-to-end exchange demo - 15 pts.** Everything working together:
  pressing S1/S2 on the display board gets a fresh batch from the sensor
  board every time, across repeated presses, without resetting either board.
- **Code submission - 10 pts.** A PR to your repo's `main` branch, with role
  owners listed in the description.

Teams of 5 split role 4's points along with its work: display role to one
person, task architecture to the other.

## Reference material

- [MSP432P401R/P401M datasheet](../datasheets/datasheet_slas826e.pdf): ADC14
  input pins and electrical specifications.
- [MSP432P4xx Technical Reference Manual](../datasheets/MSP432P4xx%20Family%20Technical%20Reference%20Manual.pdf):
  ADC14, DMA, eUSCI_A, REF_A, and CRC32 chapters.
- [MSP-EXP432P401R LaunchPad user's guide](../datasheets/MSP%E2%80%91EXP432P401R_user_guide.pdf):
  header, button, and RGB LED connections.
- [TMP35/TMP36/TMP37 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/TMP35_36_37.pdf):
  package pinout and temperature transfer function.
- SimpleLink MSP432P4 SDK examples: `adc14_single_conversion_repeat` and
  `crc32_32-bit_signature_calculation`.

<!-- cspell:ignore semphr EUSCIA eUSCI TMP36 -->
