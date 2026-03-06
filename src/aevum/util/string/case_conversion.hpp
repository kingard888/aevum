// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file case_conversion.hpp
 * @brief Defines the interfaces for high-performance string case conversion utilities.
 * @details This header file provides the declarations for functions that convert strings
 * to their uppercase and lowercase equivalents. The functions are designed for efficiency by
 * accepting `std::string_view` to avoid unnecessary allocations, while returning a new
 * `std::string` with the modified content.
 */
#pragma once

#include <string>
#include <string_view>

/**
 * @namespace aevum::util::string
 * @brief A comprehensive suite of utilities for string manipulation and processing.
 * @details This namespace contains various helper functions designed to perform common,
 * performance-sensitive string operations such as case conversion, trimming, and splitting.
 */
namespace aevum::util::string {

/**
 * @brief Transforms a string to its uppercase equivalent.
 * @details This function efficiently converts all alphabetic characters in the input string
 * to uppercase. It accepts a `std::string_view` to prevent an initial copy of the data,
 * and returns a new, owning `std::string` containing the result. The conversion is
 * locale-dependent, based on the current C locale.
 *
 * @param str The input string to convert, provided as a non-owning `std::string_view`.
 * @return A new `std::string` containing the uppercase version of the input string.
 */
[[nodiscard]] std::string to_upper(std::string_view str);

/**
 * @brief Transforms a string to its lowercase equivalent.
 * @details This function efficiently converts all alphabetic characters in the input string
 * to lowercase. It operates on a `std::string_view` to avoid unnecessary data duplication
 * and produces a new `std::string` with the converted content. The case conversion logic
 * is dependent on the currently configured C locale.
 *
 * @param str The input string to convert, provided as a non-owning `std::string_view`.
 * @return A new `std::string` containing the lowercase version of the input string.
 */
[[nodiscard]] std::string to_lower(std::string_view str);

}  // namespace aevum::util::string
