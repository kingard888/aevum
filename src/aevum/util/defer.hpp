// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file defer.hpp
 * @brief Provides a powerful, macro-driven utility for executing cleanup code at scope exit,
 * emulating the `defer` statement from the Go programming language.
 * @details This header introduces the `AEVUM_DEFER` macro and its underlying `ScopeGuard` class,
 * which leverage the principles of Resource Acquisition Is Initialization (RAII) to guarantee the
 * execution of a callable object upon leaving its enclosing scope. This is invaluable for
 * writing robust, exception-safe code, especially for resource cleanup tasks like closing files,
 * releasing locks, or freeing memory.
 */
#pragma once

#include <utility>

namespace aevum::util {

/**
 * @class ScopeGuard
 * @brief An RAII-compliant object that ensures a specified callable is executed upon destruction.
 *
 * @details This class template encapsulates a callable object (such as a lambda function) and a
 * boolean flag. Upon its construction, it stores the callable. When the `ScopeGuard` object is
 * destroyed (which occurs automatically at the end of its scope), its destructor checks the
 * active flag. If active, the stored callable is invoked. This mechanism provides a robust and
 * deterministic way to manage resources and execute cleanup actions, irrespective of how the
 * scope is exited—whether by normal execution flow, a `return` statement, a `break`, a
 * `continue`, or an exception being thrown.
 *
 * @tparam F The type of the callable object to be executed. This is typically deduced by the
 * compiler when a lambda is provided.
 */
template <typename F>
class ScopeGuard {
  public:
    /**
     * @brief Constructs a `ScopeGuard` and captures the cleanup action.
     * @details This constructor accepts any callable object via a forwarding reference, allowing
     * it to be moved or copied efficiently. The guard is initialized in an active state.
     * @param f A callable object (e.g., a lambda, function pointer, or functor) that will be
     *          invoked upon the guard's destruction.
     */
    explicit ScopeGuard(F &&f) : f_(std::forward<F>(f)), active_(true) {}

    /**
     * @brief Destructor that conditionally invokes the stored callable.
     * @details Upon destruction, this method checks if the guard is still active. If it is,
     * the captured callable `f_` is executed. This is the core of the RAII guarantee.
     */
    ~ScopeGuard() {
        if (active_) {
            f_();
        }
    }

    // The copy and move operations are explicitly deleted to enforce unique ownership and
    // guarantee that the cleanup action is executed exactly once. Allowing copies or moves
    // would introduce ambiguity about which guard instance is responsible for the cleanup,
    // leading to potential double-frees/closes or resource leaks.
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&) = delete;
    ScopeGuard &operator=(ScopeGuard &&) = delete;

    /**
     * @brief Deactivates the guard, preventing the execution of the callable upon destruction.
     * @details This method provides an escape hatch for cases where the cleanup action should be
     * conditionally suppressed. Once `dismiss()` is called, the destructor will have no effect.
     * This is useful if the ownership of a resource is transferred elsewhere before the scope ends.
     * The `noexcept` specification indicates that this operation will not throw an exception.
     */
    void dismiss() noexcept { active_ = false; }

  private:
    /**
     * @var f_
     * @brief The callable object (e.g., lambda) to be executed.
     */
    F f_;

    /**
     * @var active_
     * @brief A boolean flag that controls whether the callable `f_` should be invoked on
     * destruction. `true` if active, `false` if dismissed.
     */
    bool active_;
};

// These internal macros are a standard C++ preprocessor technique for creating unique variable
// names within a macro expansion. `AEVUM_CONCAT_IMPL` performs the token pasting, and
// `AEVUM_CONCAT` ensures that its arguments are fully expanded before being passed to
// `AEVUM_CONCAT_IMPL`. This prevents the variable name from literally being `_defer___LINE__`.
#define AEVUM_CONCAT_IMPL(s1, s2) s1##s2
#define AEVUM_CONCAT(s1, s2) AEVUM_CONCAT_IMPL(s1, s2)

/**
 * @def AEVUM_DEFER
 * @brief A convenience macro that simplifies the creation of a `ScopeGuard` for deferred execution.
 *
 * @details This macro provides a highly readable and idiomatic way to defer a block of code. It
 * instantiates a `ScopeGuard` object with a unique name based on the current line number (using
 * the `__LINE__` predefined macro). This uniqueness prevents name collisions if multiple
 * `AEVUM_DEFER` statements are used within the same scope. The provided callable is then guaranteed
 * to be executed when the scope is exited.
 *
 * @param fn A lambda expression or other callable containing the code to be executed at scope exit.
 *
 * @b Example
 * @code
 *   void process_file(const char* path) {
 *       FILE* f = fopen(path, "r");
 *       if (!f) {
 *           log_error("Failed to open file");
 *           return;
 *       }
 *       // The fclose(f) call is guaranteed to execute when process_file exits,
 *       // regardless of whether it's due to a return, an exception, or reaching the end.
 *       AEVUM_DEFER([&]() { fclose(f); });
 *
 *       // ... code that uses the file and might throw an exception or return early ...
 *   }
 * @endcode
 */
#define AEVUM_DEFER(fn) ::aevum::util::ScopeGuard AEVUM_CONCAT(_defer_, __LINE__)(fn)

}  // namespace aevum::util
