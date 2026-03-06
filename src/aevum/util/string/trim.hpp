// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file trim.hpp
 * @brief Defines the interface for a high-performance, zero-copy string trimming utility.
 * @details This header file declares a function for removing leading and trailing whitespace
 * from a string. The function is engineered for maximum efficiency by operating on
 * `std::string_view`, thus avoiding any dynamic memory allocations or data copies.
 */
#pragma once

#include <string_view>

namespace aevum::util::string {

/**
 * @brief Removes leading and trailing whitespace from a `std::string_view`.
 *
 * @details This function provides a highly efficient, allocation-free method for trimming
 * whitespace (as defined by `std::isspace` in the current locale) from both ends of a string.
 * It does not modify the original string data; instead, it returns a new `std::string_view`
 * that represents the "trimmed" subsequence of the original data. This is achieved by finding
 * the first non-whitespace character from the beginning and the first non-whitespace character
 * from the end, then creating a view that spans between them.
 *
 * @warning The returned `std::string_view` is non-owning and is only valid as long as the
 * underlying character buffer of the original string remains in scope and is not modified.
 * Accessing the view after its backing storage is invalidated will result in undefined behavior.
 *
 * @param str The string view to be trimmed.
 * @return A new `std::string_view` representing the trimmed portion of the string. If the input
 *         string consists entirely of whitespace or is empty, an empty view is returned. The
 *         function is marked `noexcept` as it is guaranteed not to throw exceptions.
 */
[[nodiscard]] std::string_view trim(std::string_view str) noexcept;

}  // namespace aevum::util::string
