# Lab 5 - ADC + UART + CRC (FreeRTOS)

For this lab, you will:

1. Implement an ADC driver, with DMA-driven sampling of a
   TMP36 analog temperature sensor.
2. Implement a UART/serial driver with an independent DMA-backed circular
   receive buffer.
3. Implement a hardware-accelerated CRC-32 driver, register-level against
   the on-chip CRC accelerator, and benchmark it against a supplied software
   CRC-32 implementation.
4. Write the ADC-to-temperature conversion yourself.
5. Use FreeRTOS to build a request/response system between two boards
   playing different roles: a **sensor** board samples the TMP36 and a
   **display** board requests readings and shows them on an RGB LED.

## Use cases

Consider this a component of a larger product. Products where a sensor node
answering on-demand requests over a CRC-protected UART link (or one like it)
would be used include:

- A central controller polling one or more remote sensor nodes on demand
  instead of being flooded with a constant stream of readings
- Environmental monitoring systems where a display/control unit is
  physically separate from the sensing hardware
- Any point-to-point serial link where corrupted data must be detected
  before it's acted on

## Overview

This lab requires FreeRTOS. Two boards play two different roles:

- The **sensor** board has the TMP36 wired to its ADC. It waits for a
  one-byte request from the display board, then samples, converts, and
  replies with a batch of readings.
- The **display** board has the RGB LED and the S1/S2 buttons. Pressing
  either button asks the sensor board for a fresh batch of readings, then
  shows the result as a color: blue for cold, yellow for warm, red for hot
  (the exact temperature bands are your call).

Both roles are implemented in the same `lab5_adc_uart_crc.cpp` file. A
`constexpr device_role this_device` near the top of the file picks which
role's tasks `main()` creates, and an `if constexpr` in `main()` compiles in
only that role's task. The other role's code is discarded at compile time,
so a single codebase produces either board's firmware. For this lab, flip
`this_device` by hand and rebuild before flashing each board; the comment
above it explains how you'd instead drive this from a compiler define on
the build system's command line if you wanted, say, a CI job to build both
roles from one invocation without hand-editing source.

The exchange itself:

1. Display board: wait for S1 or S2 to be pressed, then transmit a one-byte
   command to the sensor board.
2. Sensor board: on receiving that command, fill a 64-sample buffer from
   the TMP36 in a single DMA-backed ADC transfer, convert each raw sample to
   a temperature value, compute a CRC-32 over the batch with both your
   `crc32_hardware` driver and the supplied `crc32_software` reference
   (timing each with a `lab2::steady_clock` and printing both durations, so
   you can see which is faster), and transmit the 64 temperature values with
   the CRC appended over UART. Before transmitting, poll one of its own
   buttons (S1 or S2, your choice). If it's being held down, flip one bit
   somewhere in the outgoing packet before sending it. This doesn't need an
   interrupt; a plain level read at transmit time is enough. It gives you an
   on-demand way to demonstrate the CRC check catching corrupted data
   without editing and reflashing code.
3. Display board: receive the batch, validate its CRC, and set the RGB LED
   color from the readings. If the CRC is invalid, drop the packet, print an
   appropriate message, and turn the RGB LED magenta instead of updating it
   with a color derived from the (untrustworthy) readings.

The wire format for a single temperature value is your choice, as long as
it can resolve at least 0.5 degC steps. A plain whole-degree integer
cannot. Both boards need to agree on whatever encoding you pick, since one
side writes it and the other reads it back.

Each of these should be its own class function (or set of class functions)
added to the `adc_driver`, `serial_driver`, and `crc32_hardware` classes
within the `lab5_adc_uart_crc.cpp` file. All three drivers are written from
scratch, register-level - no DriverLib anywhere in this lab.

`adc16_stream` fills an entire buffer per call, rather than returning one
sample at a time - the ADC driver must use DMA to configure a single
transfer that fills the whole buffer, put the calling task to sleep using
FreeRTOS for the duration of that transfer, and let the DMA
transfer-complete interrupt wake it once the buffer is full (one sleep per
buffer, not one per sample). The UART driver's receive side must similarly
be backed by a circular buffer that a **separate** DMA transfer (or
interrupt) fills continuously in the background - see `receive_cursor()`'s
docs in `serial.hpp` for how a caller is expected to consume it. That means
the sensor board runs two independent DMA-backed transfers at once: one for
the ADC, one for UART reception. `crc32_software` is supplied and already
implemented; use it as-is to check your `crc32_hardware` driver's output
and as the baseline for the timing comparison.

> [!NOTE]
> Strictly speaking, this lab doesn't require FreeRTOS to work. The
> DMA-transfer-complete wake-up could just as well be done synchronously -
> the driver itself blocking on a completion flag - without an RTOS in the
> picture at all. FreeRTOS is used here anyway, mainly to get you writing
> against it before later labs depend on it more essentially.

## Learning Objectives

- Implement a buffer-filling ADC driver, register-level with DMA, that
  frees the CPU (via putting its calling task to sleep) for the duration of
  a whole buffered transfer rather than per sample.
- Implement a UART driver with a DMA-backed circular receive buffer.
- Implement a register-level driver for the on-chip CRC-32 accelerator, and
  measure how much it outperforms a software CRC-32 implementation.
- Convert a raw ADC sample from an analog temperature sensor into an actual
  temperature value.
- Design a simple request/response protocol between two independently
  running devices with different roles.
- Use `if constexpr` on a compile-time constant to build one codebase into
  either of two different firmware images.
- Detect corrupted data using a checksum computed on both ends of a link.

## Hardware

Board: MSP-EXP432P401R LaunchPad. You will need two boards - one per role.
Which ADC channel and UART instance/pins you use are up to you - check the
datasheet's pin-to-peripheral function table for which pins carry ADC14 and
eUSCI_A UART signals, and cross-connect the two boards' TX/RX lines (and
grounds). The display board also needs the on-board RGB LED and S1/S2, and
the sensor board needs one of S1/S2 (your choice which) for the corrupted-
data demonstration - see
[lab1_gpio/README.md](../lab1_gpio/README.md#hardware) for that wiring.

Sensor: [TMP36](https://www.analog.com/media/en/technical-documentation/data-sheets/TMP35_36_37.pdf)
analog temperature sensor. It outputs a voltage linearly proportional to
temperature (500 mV at 0 degC, +10 mV/degC) - wire its output to the ADC
channel you choose on the sensor board.

## Grading Rubric (100 pt)

1. **ADC + DMA - 15 pts.** `adc_driver` correctly implements the
   `adc16_stream` interface register-level with DMA, filling the whole given
   buffer in a single transfer and putting its calling task to sleep until
   the DMA transfer-complete interrupt wakes it.
2. **UART + DMA - 15 pts.** Serial driver transmits data at the configured
   baud rate, and its receive side is a circular buffer that works with its
   own independent DMA transfer.
3. **CRC driver - 15 pts.** `crc32_hardware` produces a CRC that matches the
   supplied `crc32_software` reference implementation, and is measurably
   faster.
4. **ADC-to-temperature conversion - 10 pts.** Raw ADC samples are
   correctly converted to temperature values that resolve at least 0.5 degC.
5. **Sensor role - 10 pts.** On receiving the request byte, the sensor
   board samples, converts, and transmits a 64-value batch with a valid CRC
   appended.
6. **Display role - 10 pts.** Pressing S1 or S2 requests a batch and, once
   a valid batch arrives, sets the RGB LED color based on the readings.
   Invalid CRCs are dropped with a printed message and the LED turns
   magenta instead of updating with a color derived from the readings.
7. **FreeRTOS application - 10 pts.** Each board runs the correct role's
   task, selected via the `if constexpr` in `main()`.
8. **Corrupted data demonstration - 5 pts.** Holding S1 or S2 (your choice)
   on the sensor board while it transmits flips one bit in the outgoing
   packet, and the display board correctly detects and reports the
   resulting CRC mismatch.
9. **Code submission - 10 pts.** Submit a PR to your git repo targeting your
   `main` branch.

## Reference material

- **[MSP432P401R datasheet](../datasheets/datasheet_slas826e.pdf)** -
  pin-to-peripheral function table for ADC and UART pins.
- **[MSP432P4xx Family Technical Reference Manual](../datasheets/MSP432P4xx%20Family%20Technical%20Reference%20Manual.pdf)** -
  ADC14, DMA, eUSCI_A UART mode, and CRC32 accelerator chapters.
- **[TMP36 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/TMP35_36_37.pdf)** -
  output voltage vs. temperature conversion.
