# freertos_kernel

Shared CCS project that builds the FreeRTOS kernel as a static library
(`libfreertos_kernel.a`) for the MSP432P401R. Any lab that needs FreeRTOS
references this project (see that lab's `.project`) and links against its
output - you shouldn't need to build this one directly.

## What's vendored here vs. referenced from the SDK

- **`FreeRTOS/Source/`** - the actual FreeRTOS kernel (`tasks.c`, `queue.c`,
  `list.c`, `timers.c`, `croutine.c`, `event_groups.c`, `stream_buffer.c`),
  the Cortex-M4F GCC port (`portable/GCC/ARM_CM4F/`), the `heap_4`
  allocator (`portable/MemMang/`), and the public FreeRTOS headers
  (`Source/include/`). Vendored directly into this repo because - unlike
  the MSPM0 SDK - the SimpleLink MSP432 SDK does not bundle FreeRTOS
  itself; it only ships glue code that expects an external FreeRTOS
  install. Pulled from the official
  [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) repo at
  tag `V10.1.1` (MIT licensed - see the license header at the top of each
  file), matching the version TI's own SDK build scripts were written
  against.
- **`dpl/`, `posix/`, `freertos/startup/startup_msp432_gcc.c`,
  `FreeRTOSConfig.h`** - TI's device porting layer, POSIX/pthread shim, and
  board-specific FreeRTOS config for the MSP432P401R. These are referenced
  directly from your installed SimpleLink MSP432 SDK (`kernel/freertos/...`
  and `source/ti/posix/freertos/...`) as linked resources rather than
  vendored, the same way the other labs reference DriverLib from the SDK
  instead of copying it into the repo.

## Reference material

- [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) - upstream
  kernel source and documentation.
- `kernel/freertos/builds/MSP_EXP432P401R/release/` in the SimpleLink
  MSP432 SDK - TI's own reference build of this same library, which this
  project's `.cproject` settings are modeled on.
