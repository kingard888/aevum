// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file rbac.cpp
 * @brief Implements the deterministic, state-free logic for the Role-Based Access Control (RBAC)
 * security model.
 * @details This translation unit contains the implementation of the `has_permission` function,
 * which forms the heart of the database's authorization mechanism. The logic herein is
 * intentionally simple, stateless, and hardcoded to provide a clear, auditable, and
 * high-performance permission evaluation path.
 */
#include "aevum/db/auth/rbac.hpp"

namespace aevum::db::auth {

/**
 * @brief Determines if a given user role possesses the necessary privileges to perform a specific
 * action.
 * @details This function implements a static, hierarchical Role-Based Access Control model. The
 * permission structure is as follows:
 *
 * 1.  **`UserRole::NONE`**: This role has no permissions under any circumstances. It serves as a
 *     fail-safe for unauthenticated or unauthorized access attempts.
 * 2.  **`UserRole::ADMIN`**: This role possesses omnipotent privileges. It is granted unconditional
 *     access to all actions, effectively bypassing all subsequent permission checks. This role is
 *     reserved for system-level administrative tasks.
 * 3.  **`UserRole::READ_WRITE`**: This role is granted permissions for all standard data
 * manipulation (CRUD) operations, including `find`, `count`, `insert`, `update`, `upsert`, and
 * `delete`. It represents a standard data operator.
 * 4.  **`UserRole::READ_ONLY`**: This role is the most restrictive, granting permissions
 * exclusively for non-mutating data retrieval operations (`find` and `count`).
 *
 * The function evaluates these rules sequentially, starting with the most fundamental denials and
 * proceeding to the most privileged grants.
 *
 * @param role The `UserRole` enumerator of the principal whose permissions are being checked.
 * @param action A `std::string_view` identifying the canonical name of the operation being
 * attempted.
 * @return `true` if the role's privilege level is sufficient for the requested action, otherwise
 * `false`.
 */
bool has_permission(UserRole role, std::string_view action) noexcept {
    // The `UserRole::NONE` role is explicitly denied all permissions as a foundational security
    // measure.
    if (role == UserRole::NONE) {
        return false;
    }

    // The `UserRole::ADMIN` possesses universal permissions, granting immediate access to any
    // action. This check is performed early to provide a fast path for administrative operations.
    if (role == UserRole::ADMIN) {
        return true;
    }

    // The `UserRole::READ_WRITE` is permitted to execute the full suite of standard CRUD
    // operations.
    if (role == UserRole::READ_WRITE) {
        return action == "find" || action == "count" || action == "insert" || action == "update" ||
               action == "upsert" || action == "delete";
    }

    // The `UserRole::READ_ONLY` is restricted to non-mutating data query operations.
    if (role == UserRole::READ_ONLY) {
        return action == "find" || action == "count";
    }

    // A fall-through case ensures that any future, unhandled roles will default to denying
    // permission.
    return false;
}

}  // namespace aevum::db::auth
