// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file auth_manager.hpp
 * @brief Defines the `AuthManager` class, a high-performance, thread-safe component for
 * in-memory caching and verification of user credentials.
 * @details This header file declares the primary class responsible for authentication within
 * AevumDB. The `AuthManager` is engineered to provide fast, concurrent credential validation by
 * leveraging a reader-writer lock and storing API keys in a hashed format for security. Its
 * interface is designed to be minimal yet sufficient for loading user data, performing
 * authentication checks, and querying its state.
 */
#pragma once

#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aevum/db/auth/role.hpp"

/**
 * @namespace aevum::db::auth
 * @brief Provides a centralized namespace for all components related to authentication and
 * authorization within the AevumDB engine.
 */
namespace aevum::db::auth {

/**
 * @class AuthManager
 * @brief A high-throughput, thread-safe manager for an in-memory cache of hashed API keys
 * and their associated user roles.
 *
 * @details The `AuthManager` serves as the gatekeeper for user authentication. It is designed for
 * high-concurrency environments where authentication checks are frequent. By employing a
 * `std::shared_mutex`, it optimizes for read-heavy workloads, allowing numerous `authenticate`
 * calls to proceed in parallel without contention. Write operations, such as adding a new user
 * via `add_user`, acquire an exclusive lock to ensure the atomicity and consistency of the
 * underlying cache.
 *
 * To enhance security, all API keys are stored in a hashed representation within an
 * `std::unordered_map`. This prevents plain-text credentials from ever residing in memory
 * post-initialization. The class is explicitly non-copyable to prevent accidental duplication
 * of the sensitive credential state.
 */
class AuthManager {
  public:
    /**
     * @brief Constructs a new, empty `AuthManager` instance.
     * @details The default constructor initializes the internal credential cache to an empty state,
     * ready for population from a persistent source.
     */
    AuthManager() = default;

    /**
     * @brief Default destructor.
     * @details Cleans up all resources held by the `AuthManager`, including the user cache.
     */
    ~AuthManager() = default;

    /**
     * @brief Deleted copy constructor.
     * @details The `AuthManager` is non-copyable to enforce a single source of truth for user
     * credentials and to prevent insecure duplication of the authentication state.
     */
    AuthManager(const AuthManager &) = delete;

    /**
     * @brief Deleted copy assignment operator.
     * @details The `AuthManager` is non-copyable to enforce a single source of truth for user
     * credentials and to prevent insecure duplication of the authentication state.
     * @return A reference to the current instance.
     */
    AuthManager &operator=(const AuthManager &) = delete;

    /**
     * @brief Authenticates a client-provided raw API key against the in-memory cache.
     *
     * @details This is the primary, high-performance method for validating user credentials. It
     * first hashes the incoming `raw_key` using the system's standard hashing algorithm (DJB2).
     * It then performs a highly concurrent, read-locked lookup in the internal cache.
     *
     * @param raw_key A `std::string_view` of the plain-text API key submitted by the client.
     *        The view provides an efficient, non-owning reference to the key data.
     * @return The corresponding `UserRole` if the hashed key is found in the cache and is valid.
     * @return `UserRole::NONE` if the key is empty, not found, or otherwise invalid, effectively
     *         denying access.
     */
    [[nodiscard]] UserRole authenticate(std::string_view raw_key) const;

    /**
     * @brief Atomically adds a pre-hashed user key and its associated role to the authentication
     * cache.
     *
     * @details This method is primarily intended for use during the database's initialization
     * phase, where user credentials are loaded from a persistent data store. To ensure thread
     * safety and data consistency, it acquires an exclusive write lock on the cache, serializing
     * this operation with respect to all other read and write activities.
     *
     * @param hashed_key A `std::string` containing the already-hashed representation of the user's
     *        API key. The manager takes ownership of this string.
     * @param role The `UserRole` to be associated with this key.
     */
    void add_user(std::string hashed_key, UserRole role);

    /**
     * @brief Performs a thread-safe check to determine if the authentication cache is empty.
     * @details This function is useful for determining if the database needs to bootstrap a default
     * administrator account upon startup. It acquires a shared read lock to safely inspect the
     * cache's state.
     * @return `true` if the credential cache contains no user entries, `false` otherwise.
     */
    [[nodiscard]] bool empty() const noexcept;

  private:
    /**
     * @var auth_cache_
     * @brief The core data structure for the authentication cache. It is an unordered map that
     * provides average O(1) lookup time, mapping a DJB2-hashed API key string to a `UserRole` enum.
     */
    std::unordered_map<std::string, UserRole> auth_cache_;

    /**
     * @var rw_lock_
     * @brief A mutable reader-writer mutex that provides granular, thread-safe access to
     * `auth_cache_`. It allows multiple concurrent readers (`authenticate`, `empty`) but ensures
     * that writers (`add_user`) have exclusive access, preventing data races and ensuring cache
     * integrity. It is marked `mutable` to allow locking within `const` member functions.
     */
    mutable std::shared_mutex rw_lock_;
};

}  // namespace aevum::db::auth
