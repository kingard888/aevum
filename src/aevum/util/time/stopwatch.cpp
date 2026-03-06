// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file stopwatch.cpp
 * @brief Implements the concrete logic for the high-resolution `Stopwatch` timing utility.
 * @details This source file provides the definitions for the `Stopwatch` class methods. It contains
 * the implementation of the start, stop, and reset logic, as well as the calculations for
 * reporting elapsed time in various units.
 */
#include "aevum/util/time/stopwatch.hpp"

namespace aevum::util::time {

/**
 * @brief Constructs a `Stopwatch` and immediately begins timing.
 * @details The constructor ensures that the stopwatch is in a valid, running state from the
 * moment of its creation by delegating to the `start()` method.
 */
Stopwatch::Stopwatch() noexcept { start(); }

/**
 * @brief Starts or resumes the stopwatch's measurement interval.
 * @details This method checks if the stopwatch is already running. If it is not, it captures the
 * current time point from the high-resolution clock, stores it as the new start time for the
 * interval, and sets the `running_` flag to `true`. This idempotency prevents accidental
 * resets of the start time during an active timing interval.
 */
void Stopwatch::start() noexcept {
    if (!running_) {
        start_time_ = Clock::now();
        running_ = true;
    }
}

/**
 * @brief Stops or pauses the stopwatch's measurement interval.
 * @details If the stopwatch is in a running state, this method captures the current time point.
 * It then calculates the duration of the just-completed interval by subtracting `start_time_`
 * from the current time. This duration is cast to nanoseconds for consistent internal storage
 * and added to the `accumulated_duration_`. Finally, the `running_` flag is set to `false`.
 * If the stopwatch is already stopped, this method does nothing.
 */
void Stopwatch::stop() noexcept {
    if (running_) {
        auto end_time = Clock::now();
        accumulated_duration_ +=
            std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
        running_ = false;
    }
}

/**
 * @brief Resets the stopwatch, clearing all accumulated time and stopping it.
 * @details This method provides a clean slate for the stopwatch. It sets the `running_` flag to
 * `false` and resets the `accumulated_duration_` to zero. Any subsequent call to `start()` will
 * begin a completely new timing session.
 */
void Stopwatch::reset() noexcept {
    running_ = false;
    accumulated_duration_ = std::chrono::nanoseconds(0);
}

/**
 * @brief Gets the total elapsed time in nanoseconds, accounting for the current state.
 * @details This method provides a comprehensive view of the total elapsed time. If the stopwatch
 * is currently running, it calculates the duration of the current, un-stopped interval and adds
 * it to the `accumulated_duration_` from all previous intervals. If the stopwatch is stopped,
 * it simply returns the total `accumulated_duration_`. This ensures an accurate, up-to-the-moment
 * reading regardless of the stopwatch's state.
 * @return The total elapsed time as a 64-bit unsigned integer of nanoseconds.
 */
uint64_t Stopwatch::elapsed_ns() const noexcept {
    if (running_) {
        auto current_time = Clock::now();
        auto current_duration =
            std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time_);
        return (accumulated_duration_ + current_duration).count();
    }
    return accumulated_duration_.count();
}

/**
 * @brief A convenience method to get the total elapsed time in microseconds.
 * @details This method invokes `elapsed_ns()` to get the base duration and then converts the
 * result to a floating-point value representing microseconds.
 * @return The total elapsed time in microseconds as a `double`.
 */
double Stopwatch::elapsed_us() const noexcept { return static_cast<double>(elapsed_ns()) / 1000.0; }

/**
 * @brief A convenience method to get the total elapsed time in milliseconds.
 * @details This method invokes `elapsed_ns()` to get the base duration and then converts the
 * result to a floating-point value representing milliseconds.
 * @return The total elapsed time in milliseconds as a `double`.
 */
double Stopwatch::elapsed_ms() const noexcept {
    return static_cast<double>(elapsed_ns()) / 1'000'000.0;
}

}  // namespace aevum::util::time
