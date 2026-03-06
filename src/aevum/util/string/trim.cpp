// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file trim.cpp
 * @brief Implements the concrete logic for the high-performance, zero-copy string trimming utility.
 * @details This source file provides the definition for the `trim` function. It uses standard
 * library algorithms (`std::find_if_not`) to efficiently locate the boundaries of the
 * non-whitespace content within a string view.
 */
#include "aevum/util/string/trim.hpp"

#include <algorithm>
#include <cctype>

namespace aevum::util::string {

/**
 * @brief Removes leading and trailing whitespace from a `std::string_view` without any memory
 * allocation.
 * @details This function implements a highly efficient, non-mutating trim operation. It operates
 * by first scanning from the beginning of the string view to find the first character that is not
 * classified as whitespace by `std::isspace`. It then performs a similar scan from the end of the
 * string view, using reverse iterators, to find the last non-whitespace character.
 *
 * A new `std::string_view` is then constructed from the iterators pointing to these two boundary
 * characters. This approach is "zero-copy" because it does not create a new string; it simply
 * creates a new view that references a sub-region of the original data.
 *
 * @param str The `std::string_view` to be trimmed.
 * @return A new `std::string_view` that points to the trimmed portion of the original data.
 *         If the input string consists entirely of whitespace or is empty, an empty view is
 *         returned. The returned view's validity is tied to the lifetime of the original string
 * data.
 */
std::string_view trim(std::string_view str) noexcept {
    // Use `std::find_if_not` to efficiently locate the first character that does not satisfy
    // the `std::isspace` predicate. This marks the beginning of the trimmed content.
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
        return std::isspace(c);
    });

    // A crucial edge case: if `start` reaches `str.end()`, it means the entire string
    // consists of whitespace (or is empty). In this scenario, we return an empty view.
    if (start == str.end()) {
        return {};
    }

    // To find the end of the trimmed content, we use reverse iterators (`rbegin`, `rend`).
    // `std::find_if_not` is again used, this time to find the first non-whitespace character
    // from the back of the string. The `.base()` method is then called on the resulting
    // reverse iterator to convert it back to a corresponding forward iterator that points
    // one position *past* the character that was found.
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
                   return std::isspace(c);
               }).base();

    // A new `std::string_view` is constructed from the range defined by the `start` iterator
    // and the distance between the `start` and `end` iterators.
    return std::string_view(start, std::distance(start, end));
}

}  // namespace aevum::util::string
