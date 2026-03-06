// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file rbac.hpp
 * @brief Declares the core verification function for the Role-Based Access Control (RBAC) security
 * model.
 * @details This header provides the interface for the central permission-checking mechanism in
 * AevumDB. The `has_permission` function serves as the gatekeeper for all database actions,
 * determining access rights based on a user's assigned role and the operation they intend to
 * perform.
 */
#pragma once

#include <string_view>

#include "aevum/db/auth/role.hpp"

namespace aevum::db::auth {

/**
 * @brief Adjudicates whether a user, identified by their role, is authorized to execute a specific
 * database action.
 * @details This function is the core of the RBAC engine. It encapsulates the complete set of
 * permission rules, mapping roles to their allowed actions. It is designed to be a
 * high-performance, `noexcept` function that can be called frequently at the entry point of every
 * database operation without introducing significant overhead.
 *
 * @param role The `UserRole` of the authenticated principal attempting the action. This value is
 *        the result of a successful authentication and determines the set of permissions to be
 * applied.
 * @param action A `std::string_view` representing the canonical name of the database operation
 *        being requested (e.g., "insert", "find", "create_index"). The function performs a
 *        case-sensitive comparison against this identifier.
 * @return `true` if the specified `role` possesses the necessary permissions to perform the given
 *         `action`.
 * @return `false` if the `role` is insufficient for the action, is `UserRole::NONE`, or if the
 *         action itself is not recognized by the current policy.
 */
[[nodiscard]] bool has_permission(UserRole role, std::string_view action) noexcept;

}  // namespace aevum::db::auth
