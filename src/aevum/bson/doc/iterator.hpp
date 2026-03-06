// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file iterator.hpp
 * @brief Defines a C++-style iterator for traversing elements within a BSON document.
 * @details This header file provides the `Iterator` class, which wraps `libbson`'s `bson_iter_t`
 * to offer a standard C++ iterator interface. This allows for idiomatic, range-based for loops
 * and standard algorithm compatibility when working with BSON documents, enhancing both safety
 * and readability.
 */
#pragma once

#include <bson/bson.h>
#include <iterator>
#include <string_view>

#include "aevum/bson/doc/document.hpp"

namespace aevum::bson::doc {

/**
 * @class Iterator
 * @brief A C++ forward iterator for inspecting the key-value elements of a BSON `Document`.
 *
 * @details This class provides a C++-idiomatic abstraction over the `bson_iter_t` from `libbson`.
 * It allows developers to traverse the elements of a BSON document using familiar iterator
 * patterns, including pre-increment, dereferencing, and comparison. This enables the use of
 * range-based for loops, which greatly simplifies document inspection code.
 *
 * The iterator is lightweight and primarily holds a `bson_iter_t` structure. Dereferencing the
 * iterator does not return a value but instead makes the iterator itself the source for querying
 * the current element's properties (key, type, value).
 *
 * @b Example
 * @code
 *   aevum::bson::doc::Document my_doc = ...;
 *   for (const auto& element : my_doc) {
 *       std::cout << "Key: " << element.key() << ", Type: " << (int)element.type() << std::endl;
 *   }
 * @endcode
 */
class Iterator {
  public:
    // C++ iterator traits
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Iterator;  // Dereferencing returns the iterator itself
    using difference_type = std::ptrdiff_t;
    using pointer = const Iterator *;
    using reference = const Iterator &;

    /**
     * @brief Default constructor for an invalid/end iterator.
     */
    Iterator() = default;

    /**
     * @brief Constructs an iterator, initializing it to a specific document.
     * @param doc The document to iterate over.
     */
    explicit Iterator(const Document &doc);

    /**
     * @brief Retrieves the key of the element at the current iterator position.
     * @return A `std::string_view` of the current element's key.
     */
    [[nodiscard]] std::string_view key() const;

    /**
     * @brief Retrieves the BSON type of the element at the current iterator position.
     * @return The `bson_type_t` of the current element.
     */
    [[nodiscard]] bson_type_t type() const;

    // Overloaded operators to fulfill C++ iterator requirements
    reference operator*() const noexcept { return *this; }
    pointer operator->() const noexcept { return this; }
    Iterator &operator++();
    Iterator operator++(int);

    friend bool operator==(const Iterator &lhs, const Iterator &rhs);
    friend bool operator!=(const Iterator &lhs, const Iterator &rhs);

  private:
    bson_iter_t iter_{};
    bool valid_{false};  // Flag to distinguish between a valid iterator and an end iterator
};

// Standalone begin/end functions to enable range-based for loops
Iterator begin(const Document &doc);
Iterator end(const Document &doc);

}  // namespace aevum::bson::doc
