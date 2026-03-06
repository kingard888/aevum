// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file split.cpp
 * @brief Implements the concrete logic for the high-performance, zero-copy string splitting
 * utility.
 * @details This source file provides the definition for the `split` function, which tokenizes a
 * `std::string_view` based on a character delimiter without performing any memory allocations
 * for the tokens themselves.
 */
#include "aevum/util/string/split.hpp"

namespace aevum::util::string {

/**
 * @brief Splits a `std::string_view` into a vector of tokens based on a character delimiter.
 * @details This function offers a highly performant, "zero-copy" method for tokenizing a string.
 * It iteratively scans the input `string_view` to locate occurrences of the specified delimiter.
 * For each segment found between delimiters (or between the start/end of the string and a
 * delimiter), it constructs a `std::string_view` that refers to that segment within the original
 * data buffer. This avoids the significant overhead of allocating new `std::string` objects for
 * each token, making it ideal for performance-sensitive parsing operations.
 *
 * @param str The `std::string_view` that is to be split.
 * @param delimiter The character that will be used to demarcate tokens.
 * @return A `std::vector<std::string_view>` containing the resulting tokens. The string views
 *         in this vector are non-owning and are only valid for as long as the original character
 *         buffer, from which `str` was created, remains valid and unmodified.
 */
std::vector<std::string_view> split(std::string_view str, char delimiter) {
    std::vector<std::string_view> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);

    // Loop through the string, finding each occurrence of the delimiter.
    while (end != std::string_view::npos) {
        // Construct a string_view for the token found between the `start` position and
        // the `end` position (the location of the delimiter).
        tokens.emplace_back(str.substr(start, end - start));
        // Advance the `start` position to the character immediately following the delimiter,
        // preparing for the search for the next token.
        start = end + 1;
        // Find the next occurrence of the delimiter, starting from the new `start` position.
        end = str.find(delimiter, start);
    }
    // After the loop finishes, there is one final token that runs from the last `start`
    // position to the end of the string. This token must also be added to the vector.
    tokens.emplace_back(str.substr(start));
    return tokens;
}

}  // namespace aevum::util::string
