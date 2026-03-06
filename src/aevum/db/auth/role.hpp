// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file role.hpp
 * @brief Defines the foundational, strongly-typed enumeration for user access control roles
 * within the AevumDB ecosystem.
 * @details This header file is central to the database's security model. It declares the
 * `UserRole` `enum class`, which provides a clear and type-safe representation of the distinct
 * privilege levels a user can possess. It also furnishes a `constexpr` utility function for
 * converting these roles into human-readable strings, aiding in logging, debugging, and
 * administrative interfaces.
 */
#pragma once

#include <cstdint>
#include <string_view>

namespace aevum::db::auth {

/**
 * @enum UserRole
 * @brief Specifies the hierarchical set of roles for Role-Based Access Control (RBAC).
 * @details This `enum class` defines the discrete levels of privilege within the AevumDB system.
 * By using a strongly-typed enum with an explicit underlying type of `uint8_t`, it ensures type
 * safety, prevents accidental integer conversions, and maintains a minimal memory footprint. The
 * roles are defined in a clear hierarchy of increasing privilege.
 */
enum class UserRole : uint8_t {
    /**
     * @var NONE
     * @brief The zero-privilege role, representing an unauthenticated, unauthorized, or guest user.
     * A user with this role has no permissions to perform any database operations. It serves as the
     * default state and the outcome of a failed authentication attempt.
     */
    NONE = 0,
    /**
     * @var READ_ONLY
     * @brief A role granting permissions for non-mutating data retrieval operations.
     * This includes operations like `find` and `count`, allowing the user to query and inspect data
     * without the ability to alter it in any way.
     */
    READ_ONLY = 1,
    /**
     * @var READ_WRITE
     * @brief A standard user role granting permissions for all common data manipulation operations.
     * This encompasses all read operations from `READ_ONLY`, in addition to write operations such
     * as `insert`, `update`, and `delete`.
     */
    READ_WRITE = 2,
    /**
     * @var ADMIN
     * @brief The superuser role with universal, unrestricted access to all database
     * functionalities. This role bypasses standard permission checks and is intended for
     * administrative tasks, such as user management, schema alterations, index creation, and other
     * system-level configurations.
     */
    ADMIN = 3
};

/**
 * @brief Converts a `UserRole` enumerator into its canonical, human-readable string representation.
 * @details This `constexpr` utility function provides an efficient, compile-time mechanism for
 * mapping a `UserRole` value to a string literal. This is particularly valuable for producing
 * clear diagnostic messages in logs, for display in user interfaces, or for serializing user data.
 * The use of `std::string_view` ensures that no heap allocations are performed during the
 * conversion.
 *
 * @param role The `UserRole` enumerator to be converted.
 * @return A `std::string_view` literal corresponding to the role's name (e.g., "ADMIN"). If an
 *         unrecognized value is passed, it safely defaults to "NONE".
 */
[[nodiscard]] constexpr std::string_view to_string(UserRole role) noexcept {
    switch (role) {
        case UserRole::READ_ONLY:
            return "READ_ONLY";
        case UserRole::READ_WRITE:
            return "READ_WRITE";
        case UserRole::ADMIN:
            return "ADMIN";
        default:
            return "NONE";
    }
}

}  // namespace aevum::db::auth
