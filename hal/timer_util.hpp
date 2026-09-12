#pragma once

#include <chrono>
#include <cstdint>

#include "timer.hpp"

namespace lab2 {

/// Number of nanoseconds in one second.
constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000ULL;

/**
 * @brief Convert a duration of time into a number of steady clock ticks
 *
 * A steady clock counts ticks, not time. To wait for a real duration you have
 * to convert that duration into "how many ticks of this specific clock is
 * that?" The conversion is:
 *
 *     ticks = duration_in_seconds * frequency
 *
 * Durations here are in nanoseconds, so that becomes:
 *
 *     ticks = (nanoseconds * frequency) / 1,000,000,000
 *
 * That multiply is the dangerous part. A 60 second delay is 6e10 nanoseconds,
 * and at 48 MHz that product is 2.8e18 - close enough to the 1.8e19 limit of a
 * 64-bit integer that a longer delay would silently overflow. So instead of
 * multiplying the whole nanosecond count at once, the duration is split into
 * whole seconds plus a leftover remainder. The whole seconds can be multiplied
 * by the frequency directly, and the remainder is always less than 1e9, so
 * neither product can overflow at any frequency this device supports.
 *
 * @param p_frequency - operating frequency of the steady clock in hertz
 * @param p_duration - amount of time to convert
 * @return std::uint64_t - equivalent number of ticks. Returns 0 for a zero or
 * negative duration. Any positive duration returns at least 1 tick, so that a
 * duration shorter than a single tick still waits rather than returning
 * instantly.
 */
[[nodiscard]] inline std::uint64_t duration_to_ticks(
  std::uint32_t p_frequency,
  std::chrono::nanoseconds p_duration)
{
  if (p_duration <= std::chrono::nanoseconds(0)) {
    return 0;
  }

  auto const nanoseconds = static_cast<std::uint64_t>(p_duration.count());
  auto const frequency = static_cast<std::uint64_t>(p_frequency);

  auto const whole_seconds = nanoseconds / nanoseconds_per_second;
  auto const remainder = nanoseconds % nanoseconds_per_second;

  // whole_seconds is already in seconds, so multiplying by frequency (ticks
  // per second) lands on ticks directly. remainder is still in nanoseconds,
  // so only it needs the nanosecond-to-second conversion.
  auto const whole_ticks = whole_seconds * frequency;
  auto const remainder_ticks = (remainder * frequency) / nanoseconds_per_second;
  auto const ticks = whole_ticks + remainder_ticks;

  return (ticks == 0) ? 1 : ticks;
}

/**
 * @brief Compute the uptime value that a steady clock will reach after a
 * duration has elapsed
 *
 * The returned deadline is only meaningful for the steady clock passed in,
 * because it is expressed in that clock's ticks and is measured from that
 * clock's current uptime.
 *
 * @param p_steady_clock - clock to compute the deadline against
 * @param p_duration - how far into the future the deadline should be
 * @return std::uint64_t - uptime value the clock will have reached once the
 * duration has passed
 */
[[nodiscard]] inline std::uint64_t future_deadline(
  steady_clock& p_steady_clock,
  std::chrono::nanoseconds p_duration)
{
  auto const ticks = duration_to_ticks(p_steady_clock.frequency(), p_duration);
  return p_steady_clock.uptime() + ticks;
}

/**
 * @brief Block the caller until a duration has elapsed
 *
 * This is a busy wait. The CPU spins reading the clock and does nothing else
 * until the deadline is reached.
 *
 * Durations are easiest to write with the chrono literals:
 *
 * ```cpp
 * using namespace std::chrono_literals;
 *
 * lab2::delay(clock, 500ms);
 * lab2::delay(clock, 20us);
 * lab2::delay(clock, 2s);
 * ```
 *
 * The accuracy of this delay is limited by the resolution of the clock you
 * give it. A 3 MHz clock ticks every 333 ns, so it cannot resolve a delay
 * finer than that.
 *
 * @param p_steady_clock - clock used to measure the passage of time
 * @param p_duration - how long to block for
 */
inline void delay(steady_clock& p_steady_clock,
                  std::chrono::nanoseconds p_duration)
{
  auto const deadline = future_deadline(p_steady_clock, p_duration);
  while (p_steady_clock.uptime() < deadline) {
    continue;
  }
}
}  // namespace lab2
