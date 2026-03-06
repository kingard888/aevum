// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file thread_name.cpp
 * @brief Implements the cross-platform utilities for assigning descriptive names to execution
 * threads.
 * @details This source file provides the concrete, platform-dependent implementations for the
 * `set_current_thread_name` function. It uses preprocessor directives to select the appropriate
 * native API for interacting with the host operating system's thread management facilities,
 * ensuring that the high-level abstraction functions correctly across different environments.
 */
#include "aevum/util/concurrency/thread_name.hpp"

// Conditionally include the POSIX thread library header for platforms that support it.
#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#endif

namespace aevum::util::concurrency {

/**
 * @brief Assigns a specified name to the currently executing thread by dispatching to the
 * appropriate OS-specific implementation.
 * @details This function serves as a portable wrapper for the inherently non-portable operation
 * of setting a thread's name. Such an operation is invaluable for debugging and profiling, as
 * it annotates anonymous system threads with human-readable identifiers. The implementation
 * internally handles platform-specific idiosyncrasies, such as API function names and limitations
 * on name length, providing a clean and unified interface to the developer.
 *
 * @param name The desired name for the current thread. This name may be truncated to comply with
 * underlying operating system constraints.
 */
void set_current_thread_name(const std::string &name) {
#if defined(__linux__)
    // On Linux, the pthread_setname_np function is used. The 'np' suffix denotes that this is a
    // non-portable extension, common in glibc. The kernel imposes a strict limit of 16 bytes
    // (including the null terminator) for the thread name. Therefore, the input string is
    // truncated to 15 characters to ensure compliance.
    std::string truncated_name = name.substr(0, 15);
    pthread_setname_np(pthread_self(), truncated_name.c_str());

#elif defined(__APPLE__)
    // On macOS, the function is also named pthread_setname_np, but it only accepts a single
    // argument: the name to be set for the *current* thread. The name length limit is more
    // generous than on Linux, typically 64 bytes. We truncate to 63 to be safe.
    std::string truncated_name = name.substr(0, 63);
    pthread_setname_np(truncated_name.c_str());

#else
    // For operating systems where a thread-naming mechanism is not supported or has not been
    // implemented (e.g., older versions of Windows without SetThreadDescription), this function
    // gracefully degrades into a no-op. The parameter is explicitly cast to void to suppress
    // potential "unused parameter" warnings from the compiler, indicating that the parameter
    // is intentionally ignored on this platform.
    (void)name;
#endif
}

}  // namespace aevum::util::concurrency
