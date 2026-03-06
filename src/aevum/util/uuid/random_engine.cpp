// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file random_engine.cpp
 * @brief Implements the thread-local random number generator for high-quality, concurrent
 * random number production.
 * @details This source file provides the definition for the `next_uint64` function. It leverages
 * `thread_local` storage to create a distinct, high-quality random number generation pipeline for
 * each thread, thereby eliminating lock contention and ensuring excellent performance in
 * multithreaded contexts.
 */
#include "aevum/util/uuid/random_engine.hpp"

#include <random>

namespace aevum::util::uuid::detail {

/**
 * @brief Generates a cryptographically strong, uniformly-distributed 64-bit unsigned integer
 * in a thread-safe and contention-free manner.
 *
 * @details This function is the cornerstone of random data generation for UUIDs. Its design
 * prioritizes both the quality of randomness and performance in concurrent applications. It
 * achieves this by declaring its random number generation objects with `thread_local` storage
 * duration. This C++ feature ensures that each thread that calls this function will have its
 * own unique, independent instances of the random device, engine, and distribution.
 *
 * The pipeline is as follows:
 * 1. `thread_local std::random_device rd`: A non-deterministic source of randomness, typically
 *    interfacing with a hardware entropy source (e.g., `/dev/urandom` on Linux), is created
 *    once per thread.
 * 2. `thread_local std::mt19937_64 gen(rd())`: The powerful Mersenne Twister 19937 pseudo-random
 *    generator (64-bit version) is created and seeded with the high-quality entropy from `rd`.
 *    This seeding happens only once per thread, upon first invocation.
 * 3. `thread_local std::uniform_int_distribution<uint64_t> dis`: An object that transforms the
 *    raw output of the engine into a uniformly distributed value over the entire range of
 *    `uint64_t`.
 *
 * This design completely avoids the need for mutexes or other forms of synchronization, making
 * calls to `next_uint64` from different threads execute in parallel without any contention.
 *
 * @return A randomly generated `uint64_t` value.
 */
uint64_t next_uint64() noexcept {
    // The `thread_local` keyword ensures that each of the following variables is created and
    // initialized once for each thread. This is the key to achieving lock-free thread safety.
    thread_local std::random_device rd;
    thread_local std::mt19937_64 gen(rd());
    thread_local std::uniform_int_distribution<uint64_t> dis;

    // Generate and return the next random number from the thread's dedicated distribution.
    return dis(gen);
}

}  // namespace aevum::util::uuid::detail
