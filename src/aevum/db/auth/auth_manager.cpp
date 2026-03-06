// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file auth_manager.cpp
 * @brief Implements the high-concurrency `AuthManager` class for handling user authentication.
 * @details This file provides the concrete implementations for the methods declared in
 * `auth_manager.hpp`. It leverages a `std::shared_mutex` to achieve a highly efficient
 * reader-writer lock pattern, optimizing for frequent authentication checks while ensuring safe,
 * atomic updates to the credential cache.
 */
#include "aevum/db/auth/auth_manager.hpp"

#include <mutex>
#include <shared_mutex>

#include "aevum/util/hash/djb2.hpp"
#include "aevum/util/log/logger.hpp"

namespace aevum::db::auth {

/**
 * @brief Validates a raw, plain-text API key by hashing and checking it against the in-memory
 * cache.
 * @details This function orchestrates the core authentication workflow.
 *
 * First, it performs a preliminary check to reject empty keys immediately, avoiding unnecessary
 * hashing and lookup operations. The provided `raw_key` is then transformed into a secure hash
 * using the `aevum::util::hash::djb2_string` algorithm.
 *
 * To ensure high throughput, a `std::shared_lock` is acquired on the `rw_lock_`. This optimistic
 * read lock permits multiple threads to execute this function concurrently, maximizing performance
 * under typical read-heavy workloads where authentication checks far outnumber user additions.
 * A lookup is then performed on the `auth_cache_`. If a match is found, the corresponding
 * `UserRole` is returned. If no match is found after scanning the cache, `UserRole::NONE` is
 * returned to signify authentication failure.
 *
 * @param raw_key A `std::string_view` representing the untrusted, plain-text API key from a client.
 * @return The `UserRole` associated with the key if the authentication is successful;
 *         `UserRole::NONE` if the key is empty, unknown, or invalid.
 */
UserRole AuthManager::authenticate(std::string_view raw_key) const {
    // Reject empty keys upfront as they are inherently invalid.
    if (raw_key.empty()) {
        return UserRole::NONE;
    }

    // Securely hash the incoming raw key before performing any lookups.
    // This ensures that plain-text credentials are never compared or stored directly.
    std::string hashed_attempt = aevum::util::hash::djb2_string(raw_key);

    // Acquire a shared (read) lock to allow for maximum concurrency during authentication checks.
    // This lock allows multiple threads to read from the auth_cache_ simultaneously.
    std::shared_lock<std::shared_mutex> read_lock(rw_lock_);

    auto it = auth_cache_.find(hashed_attempt);
    if (it != auth_cache_.end()) {
        // A matching hashed key was found; return its associated privilege level.
        return it->second;
    }

    // If the loop completes without a match, the key is not in the cache.
    return UserRole::NONE;
}

/**
 * @brief Ingests a new user's pre-hashed credentials and role into the authentication cache.
 * @details This is a write operation and therefore requires exclusive access to the `auth_cache_`.
 * It acquires a `std::unique_lock` on the `rw_lock_`, which blocks all other threads—both readers
 * and other writers—from accessing the cache until the operation is complete. This guarantees
 * that the cache remains in a consistent state and prevents data races. The function uses
 * `emplace` with `std::move` for efficient insertion of the new user record. This method is
 * typically invoked during server startup when populating the cache from persistent storage.
 *
 * @param hashed_key The DJB2-hashed representation of the user's API key. The `AuthManager`
 *        takes ownership of this string.
 * @param role The `UserRole` to be assigned to this user.
 */
void AuthManager::add_user(std::string hashed_key, UserRole role) {
    // Acquire a unique (exclusive) lock for writing. This blocks all other read and write
    // operations, ensuring the atomicity of the cache modification.
    std::unique_lock<std::shared_mutex> write_lock(rw_lock_);
    auth_cache_.emplace(std::move(hashed_key), role);
}

/**
 * @brief Atomically checks if the authentication cache contains any user entries.
 * @details This function acquires a `std::shared_lock` to ensure a thread-safe inspection
 * of the cache's state. It provides a reliable way to determine if any users have been
 * loaded, which is critical for system bootstrapping logic (e.g., creating a default
 * admin user if the database is new).
 * @return `true` if `auth_cache_` is empty, `false` otherwise.
 */
bool AuthManager::empty() const noexcept {
    // Acquire a shared (read) lock to safely check the cache's size.
    std::shared_lock<std::shared_mutex> read_lock(rw_lock_);
    return auth_cache_.empty();
}

}  // namespace aevum::db::auth
