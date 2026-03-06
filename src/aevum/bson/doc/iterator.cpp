// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file iterator.cpp
 * @brief Implements the `Iterator` class for traversing BSON documents.
 * @details This source file provides the concrete definitions for the `Iterator` class methods,
 * bridging the C-style `bson_iter_t` with a modern C++ forward iterator interface.
 */
#include "aevum/bson/doc/iterator.hpp"

namespace aevum::bson::doc {

/**
 * @brief Constructs an iterator and initializes it to the beginning of a document.
 * @details This constructor initializes the internal `bson_iter_t` to point to the first
 * element of the provided `Document`. If the document is not empty, the iterator is marked
 * as valid.
 * @param doc The document over which to iterate.
 */
Iterator::Iterator(const Document &doc) {
    if (doc.get()) {
        valid_ = bson_iter_init(&iter_, doc.get());
    }
}

/**
 * @brief Retrieves the key of the current BSON element.
 * @return A `std::string_view` of the key. Returns an empty view if the iterator is invalid.
 */
std::string_view Iterator::key() const {
    if (!valid_) {
        return {};
    }
    return bson_iter_key(&iter_);
}

/**
 * @brief Retrieves the type of the current BSON element.
 * @return The `bson_type_t` of the element. Returns `BSON_TYPE_EOD` if the iterator is invalid.
 */
bson_type_t Iterator::type() const {
    if (!valid_) {
        return BSON_TYPE_EOD;
    }
    return bson_iter_type(&iter_);
}

/**
 * @brief Pre-increment operator. Advances the iterator to the next element in the document.
 * @return A reference to the incremented iterator.
 */
Iterator &Iterator::operator++() {
    if (valid_) {
        valid_ = bson_iter_next(&iter_);
    }
    return *this;
}

/**
 * @brief Post-increment operator. Advances the iterator to the next element.
 * @return A copy of the iterator before it was incremented.
 */
Iterator Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

/**
 * @brief Compares two iterators for equality.
 * @details Two iterators are considered equal if both are invalid (at the end), or if they are
 * both valid and point to the same position in the same BSON buffer.
 * @param lhs The left-hand side iterator.
 * @param rhs The right-hand side iterator.
 * @return `true` if the iterators are equal, `false` otherwise.
 */
bool operator==(const Iterator &lhs, const Iterator &rhs) {
    if (!lhs.valid_ && !rhs.valid_) {
        return true;
    }
    if (lhs.valid_ != rhs.valid_) {
        return false;
    }
    // Compare raw buffer pointers and current offsets
    return lhs.iter_.raw == rhs.iter_.raw &&
           bson_iter_offset(const_cast<bson_iter_t *>(&lhs.iter_)) ==
               bson_iter_offset(const_cast<bson_iter_t *>(&rhs.iter_));
}

/**
 * @brief Compares two iterators for inequality.
 * @return `true` if the iterators are not equal, `false` otherwise.
 */
bool operator!=(const Iterator &lhs, const Iterator &rhs) { return !(lhs == rhs); }

/**
 * @brief Returns an iterator to the beginning of the document.
 * @param doc The document for which to get the begin iterator.
 * @return An `Iterator` pointing to the first element.
 */
Iterator begin(const Document &doc) { return Iterator(doc); }

/**
 * @brief Returns an iterator representing the end of the document.
 * @param doc The document (unused, for interface consistency).
 * @return A default-constructed "end" iterator.
 */
Iterator end(const Document & /*doc*/) { return Iterator(); }

}  // namespace aevum::bson::doc
