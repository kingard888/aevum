// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file stopwatch.hpp
 * @brief Defines a high-resolution `Stopwatch` class for precision performance profiling and
 * timing.
 * @details This header file provides the declaration for the `Stopwatch` class, a versatile utility
 * for measuring elapsed time with high accuracy. It is built upon
 * `std::chrono::high_resolution_clock` and is designed for tasks such as benchmarking code
 * sections, measuring operation latencies, and general performance monitoring.
 */
#pragma once

#include <chrono>
#include <cstdint>

/**
 * @namespace aevum::util::time
 * @brief Provides a collection of utilities for time measurement, timestamp generation, and
 * formatting.
 * @details This namespace encapsulates components designed to handle various time-related tasks,
 * from high-precision performance measurement to generating standardized timestamps.
 */
namespace aevum::util::time {

/**
 * @class Stopwatch
 * @brief Implements a precision timer for accurately measuring elapsed time durations.
 *
 * @details This class provides a simple yet powerful interface for timing events and code
 * execution. It leverages `std::chrono::high_resolution_clock` to achieve the highest possible
 * timing precision available on the platform. The stopwatch can be started, stopped (which
 * effectively pauses the measurement), resumed, and reset. This allows for the accumulation of time
 * over multiple, non-contiguous intervals, making it flexible for a wide range of profiling
 * scenarios. The internal state correctly handles both running and paused states when reporting
 * elapsed time.
 */
class Stopwatch {
  public:
    /**
     * @brief Constructs a `Stopwatch` object and immediately puts it into a running state.
     * @details The constructor initializes the stopwatch and calls `start()`, recording the
     *          initial time point.
     */
    Stopwatch() noexcept;

    /**
     * @brief Starts or resumes the stopwatch's time measurement.
     * @details If the stopwatch is currently paused, this method records a new `start_time_`
     *          and marks the stopwatch as running. If the stopwatch is already running, this
     *          call has no effect, preventing the start time from being incorrectly reset.
     */
    void start() noexcept;

    /**
     * @brief Stops (or pauses) the stopwatch.
     * @details If the stopwatch is currently running, this method records the current time as the
     *          end time for the interval, calculates the duration of the interval, and adds it
     *          to the `accumulated_duration_`. It then marks the stopwatch as not running. If the
     *          stopwatch is already stopped, this call is idempotent and has no effect.
     */
    void stop() noexcept;

    /**
     * @brief Resets the stopwatch to a zeroed and stopped state.
     * @details This method clears all accumulated time, resetting `accumulated_duration_` to zero.
     *          It also ensures the stopwatch is in a stopped state, ready for a fresh timing
     * session.
     */
    void reset() noexcept;

    /**
     * @brief Retrieves the total elapsed time in milliseconds.
     * @details This method calculates the total elapsed time, including the currently running
     *          interval if the stopwatch is active, and returns it as a `double` in milliseconds.
     * @return The total elapsed duration in milliseconds.
     */
    [[nodiscard]] double elapsed_ms() const noexcept;

    /**
     * @brief Retrieves the total elapsed time in microseconds.
     * @details This method calculates the total elapsed time, including the currently running
     *          interval if the stopwatch is active, and returns it as a `double` in microseconds.
     * @return The total elapsed duration in microseconds.
     */
    [[nodiscard]] double elapsed_us() const noexcept;

    /**
     * @brief Retrieves the total elapsed time in nanoseconds.
     * @details This method calculates the total elapsed time, including the currently running
     *          interval if the stopwatch is active, and returns it as a `uint64_t` in nanoseconds.
     *          This provides the highest resolution available.
     * @return The total elapsed duration in nanoseconds.
     */
    [[nodiscard]] uint64_t elapsed_ns() const noexcept;

  private:
    /**
     * @typedef Clock
     * @brief An alias for `std::chrono::high_resolution_clock`, the most precise clock available.
     */
    using Clock = std::chrono::high_resolution_clock;

    /**
     * @typedef TimePoint
     * @brief An alias for a time point measured by the high-resolution `Clock`.
     */
    using TimePoint = std::chrono::time_point<Clock>;

    /**
     * @var start_time_
     * @brief The time point recorded when the stopwatch was last started or resumed.
     */
    TimePoint start_time_;

    /**
     * @var accumulated_duration_
     * @brief The total time accumulated during all previous start/stop intervals, stored in
     * nanoseconds.
     */
    std::chrono::nanoseconds accumulated_duration_{0};

    /**
     * @var running_
     * @brief A boolean flag indicating the current state of the stopwatch (`true` if running,
     * `false` if stopped/paused).
     */
    bool running_{false};
};

}  // namespace aevum::util::time
