// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file split.hpp
 * @brief Defines the interface for a high-performance, zero-copy string splitting utility.
 * @details This header file declares a function for tokenizing a string based on a delimiter.
 * The implementation is designed for maximum efficiency by returning a vector of
 * `std::string_view`, which avoids dynamic memory allocations and data copying by providing views
 * into the original string.
 */
#pragma once

#include <string_view>
#include <vector>

namespace aevum::util::string {

/**
 * @brief Splits a string by a specified delimiter into a vector of non-owning string views.
 *
 * @details This function provides a high-performance, allocation-free mechanism to tokenize a
 * string. It iterates through the input `str` and identifies segments separated by the given
 * `delimiter`. For each segment, it creates a `std::string_view` that points to the
 * corresponding subsequence within the original string data. This approach is exceptionally
 * fast as it avoids the overhead of creating new `std::string` objects for each token.
 *
 * @warning The `std::string_view` objects contained in the returned vector are non-owning.
 * They are only valid as long as the underlying character buffer, from which the original
 * `str` was created, remains in scope and is not modified. Using the views after the
 * backing storage has been deallocated or changed will result in undefined behavior.
 *
 * @param str The string view to be split.
 * @param delimiter The character that serves as the boundary for splitting the string.
 * @return A `std::vector<std::string_view>`, where each view represents a token. If no
 *         delimiters are found, the vector will contain a single view of the original string.
 */
[[nodiscard]] std::vector<std::string_view> split(std::string_view str, char delimiter);

}  // namespace aevum::util::string
