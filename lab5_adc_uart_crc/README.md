# Lab 5 - ADC + UART + CRC (FreeRTOS)

For this lab, you will:

1. Implement an ADC driver using DriverLib, with DMA-driven sampling of a
   TMP36 analog temperature sensor.
2. Implement a UART/serial driver with a second, independent DMA-backed
   circular receive buffer.
3. Implement a hardware-accelerated CRC-32 driver, register-level against
   the on-chip CRC accelerator, and benchmark it against a supplied software
   CRC-32 implementation.
4. Use FreeRTOS to run ADC sampling and the UART exchange as two concurrent
   tasks connected by a queue.
5. Exchange TMP36 temperature readings with another microcontroller over
   UART, each frame protected by a CRC-32.

## Use cases

Consider this a component of a larger product. Products where a
DMA-sampled sensor feeding a CRC-protected UART exchange (or one like it)
would be used include:

- Sensor nodes reporting readings to a central controller over a wired link
- Environmental monitoring systems distributing readings between boards
- Any point-to-point serial link where corrupted data must be detected
  before it's acted on

## Overview

This lab requires FreeRTOS. Write an ADC driver against the `adc16_stream`
interface (`adc.hpp`), a serial driver against the `serial` interface
(`serial.hpp`), and a CRC-32 driver against the `crc32` interface
(`crc.hpp`), then use them from two FreeRTOS tasks:

1. An ADC task fills a 64-sample buffer from the TMP36 in a single
   DMA-backed transfer, averages the buffer, converts the average to
   degrees Celsius, and sends the result onto a queue. This task should
   sleep between samples.
2. An exchange task waits on that queue for the next reading, computes a
   CRC-32 over it using both your `crc32_hardware` driver and the supplied
   `crc32_software` reference implementation (timing both with a
   `lab2::steady_clock` and printing both durations, so you can see which
   is faster), transmits the reading with the CRC appended over UART, then
   waits to receive the other board's packet, validates its CRC, and prints
   the result. The two boards take turns this way, one after the other,
   starting and stopping the exchange on a button press.

Each of these should be its own class function (or set of class functions)
added to the `adc_driver`, `serial_driver`, and `crc32_hardware` classes
within the `lab5_adc_uart_crc.cpp` file.

`adc16_stream` fills an entire buffer per call, rather than returning one
sample at a time - the ADC driver must use DMA to configure a single
transfer that fills the whole buffer, put the calling task to sleep using
FreeRTOS for the duration of that transfer, and let the DMA
transfer-complete interrupt wake it once the buffer is full (one sleep per
buffer, not one per sample). The UART driver's receive side must similarly
be backed by a circular buffer that a **separate** DMA transfer (or
interrupt) fills continuously in the background - see `receive_cursor()`'s
docs in `serial.hpp` for how a caller is expected to consume it. That means
this lab uses two independent DMA-backed transfers at once: one for the ADC,
one for UART reception. The `crc32_hardware` driver must be written
register-level against the on-chip CRC accelerator - no DriverLib.
`crc32_software` is supplied and already implemented; use it as-is to check
your hardware driver's output and as the baseline for the timing comparison.

## Learning Objectives

- Implement a buffer-filling ADC driver, using DriverLib and DMA, that frees
  the CPU (via putting its calling task to sleep) for the duration of a
  whole buffered transfer rather than per sample.
- Implement a UART driver with a DMA-backed circular receive buffer,
  running alongside the ADC's own independent DMA transfer.
- Implement a register-level driver for the on-chip CRC-32 accelerator, and
  measure how much it outperforms a software CRC-32 implementation.
- Use FreeRTOS tasks and a queue to move data between a producer (ADC
  sampling) and a consumer (the UART exchange) running concurrently.
- Design and implement a simple point-to-point exchange protocol, gated by
  a button, between two independently-running devices.
- Detect corrupted data using a checksum computed on both ends of a link.

## Hardware

Board: MSP-EXP432P401R LaunchPad. You will need two boards to exchange data
with each other. Which ADC channel and UART instance/pins you use are up to
you - check the datasheet's pin-to-peripheral function table for which pins
carry ADC14 and eUSCI_A UART signals, and cross-connect the two boards'
TX/RX lines (and grounds).

Sensor: [TMP36](https://www.analog.com/media/en/technical-documentation/data-sheets/TMP35_36_37.pdf)
analog temperature sensor. It outputs a voltage linearly proportional to
temperature (500 mV at 0 degC, +10 mV/degC) - wire its output to the ADC
channel you choose.

## Grading Rubric (100 pt)

1. **ADC + DMA - 15 pts.** `adc_driver` correctly implements the
   `adc16_stream` interface using DriverLib and DMA, filling the whole given
   buffer in a single transfer and putting its calling task to sleep until
   the DMA transfer-complete interrupt wakes it.
2. **UART + DMA - 15 pts.** Serial driver transmits data at the configured
   baud rate, and its receive side is a circular buffer that works with its
   own independent DMA transfer.
3. **CRC driver - 15 pts.** `crc32_hardware` produces a CRC that matches the
   supplied `crc32_software` reference implementation, and is measurably
   faster.
4. **FreeRTOS application - 15 pts.** ADC and exchange tasks run
   concurrently, connected by a queue.
5. **TMP36 conversion - 10 pts.** ADC samples are correctly averaged and
   converted to degrees Celsius.
6. **End-to-end exchange - 15 pts.** Temperature readings are transmitted
   with a valid CRC appended, received, and validated on the other end. If
   the CRC is invalid, the packet is dropped and an appropriate message is
   printed to stdout.
7. **Corrupted data demonstration - 5 pts.** Lab instructor will ask you to
   update your code to corrupt a single bit in the sequence to confirm that
   the CRC checks are happening between both boards.
8. **Code submission - 10 pts.** Submit a PR to your git repo targeting your
   `main` branch.

## Reference material

- **[MSP432P401R datasheet](../datasheets/datasheet_slas826e.pdf)** -
  pin-to-peripheral function table for ADC and UART pins.
- **[MSP432P4xx Family Technical Reference Manual](../datasheets/MSP432P4xx%20Family%20Technical%20Reference%20Manual.pdf)** -
  ADC14, DMA, eUSCI_A UART mode, and CRC32 accelerator chapters.
- **[TMP36 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/TMP35_36_37.pdf)** -
  output voltage vs. temperature conversion.
- **SimpleLink MSP432 SDK DriverLib documentation** - ADC14, DMA, and UART
  (eUSCI_A) APIs.
