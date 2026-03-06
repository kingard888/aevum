// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file case_conversion.cpp
 * @brief Implements the concrete logic for string case conversion utilities.
 * @details This source file provides the definitions for the `to_upper` and `to_lower` functions.
 * It leverages the `std::transform` algorithm in conjunction with `std::toupper` and `std::tolower`
 * from the cctype library to perform efficient, in-place modification of a string copy.
 */
#include "aevum/util/string/case_conversion.hpp"

#include <algorithm>
#include <cctype>

namespace aevum::util::string {

/**
 * @brief Converts a given string to its uppercase equivalent.
 * @details This function first creates a `std::string` copy of the input `std::string_view` to
 * serve as a modifiable buffer. It then employs the `std::transform` algorithm to apply the
 * `std::toupper` function to every character in the copied string. This approach is both
 * efficient and idiomatic C++, yielding a new string with the desired case transformation.
 * The behavior of `std::toupper` is influenced by the currently active C locale.
 *
 * @param str The input `std::string_view` to be converted.
 * @return A new `std::string` object containing the uppercase version of the input data.
 */
std::string to_upper(std::string_view str) {
    std::string result(str);
    // `std::transform` iterates from `result.begin()` to `result.end()`, applies the lambda
    // function to each character, and writes the transformed character back into the result
    // string starting at `result.begin()`.
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return result;
}

/**
 * @brief Converts a given string to its lowercase equivalent.
 * @details Similar to `to_upper`, this function initializes a new `std::string` from the input
 * `std::string_view`. It then utilizes `std::transform` to apply the `std::tolower` function
 * to each character, effectively converting the entire string to lowercase. The specific
 * transformations are determined by the current C locale settings.
 *
 * @param str The input `std::string_view` to be converted.
 * @return A new `std::string` object containing the lowercase version of the input data.
 */
std::string to_lower(std::string_view str) {
    std::string result(str);
    // The algorithm processes the string in-place on the newly created `result` copy,
    // ensuring the original data pointed to by `str` remains unmodified.
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}

}  // namespace aevum::util::string
