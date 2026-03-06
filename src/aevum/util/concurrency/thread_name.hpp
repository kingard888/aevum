// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file thread_name.hpp
 * @brief Declares a cross-platform utility for assigning descriptive names to execution threads.
 * @details This header file provides the interface for a function that abstracts away the
 * platform-specific system calls required to name a thread. Naming threads is a critical practice
 * for debugging and profiling complex multithreaded applications, as it allows for the clear
 * identification of threads in system monitors, debuggers, and performance analyzers.
 */
#pragma once

#include <string>
#include <thread>

/**
 * @namespace aevum::util::concurrency
 * @brief A curated collection of advanced utilities and primitives for concurrent programming and
 * high-performance multithreading.
 * @details This namespace encapsulates a suite of tools designed to abstract away the complexities
 * of thread synchronization, atomic operations, and parallel execution, providing developers with
 * robust, reusable, and efficient building blocks for concurrent software architectures.
 */
namespace aevum::util::concurrency {

/**
 * @brief Assigns a specified name to the currently executing thread.
 *
 * @details This function provides a portable, high-level interface for setting the name of the
 * calling thread, which is then visible to the underlying operating system. This functionality
 * is indispensable for debugging, profiling, and monitoring, as it replaces opaque thread IDs
 * with human-readable identifiers in tools like `gdb`, `htop`, Visual Studio's thread window,
 * and other system diagnostic utilities. The function gracefully handles platform-specific
 * constraints, such as the name length limit imposed by the Linux kernel (15 characters plus
 * null terminator), by automatically truncating the provided name.
 *
 * @param name The desired, human-readable name to assign to the current thread. The name will
 * be truncated if it exceeds the length limitations of the target operating system.
 *
 * @b Platform-Specific-Behavior
 *   - On **Linux**, uses `pthread_setname_np`, truncating the name to 15 characters.
 *   - On **macOS**, uses `pthread_setname_np`, truncating the name to 63 characters.
 *   - On **Windows**, this would typically use `SetThreadDescription` (if available), but is a
 * no-op in this implementation.
 *   - On other unsupported platforms, the function has no effect.
 */
void set_current_thread_name(const std::string &name);

}  // namespace aevum::util::concurrency
