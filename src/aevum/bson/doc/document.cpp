// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file document.cpp
 * @brief Implements the `Document` class, providing the concrete definitions for its lifecycle
 * management and utility methods.
 * @details This source file contains the implementation of the C++ RAII wrapper for `bson_t`.
 * It defines the constructors, destructor, and assignment operators that form the core of the
 * class's resource management guarantees, ensuring that the underlying `libbson` objects are
 * handled in a safe, predictable, and exception-safe manner.
 */
#include "aevum/bson/doc/document.hpp"

#include <utility>  // Required for std::exchange

namespace aevum::bson::doc {

/**
 * @brief Default constructor for creating a new, empty BSON document.
 * @details This constructor invokes `bson_new()` to allocate a fresh, heap-based `bson_t`
 * structure. The resulting document is valid and empty, ready for elements to be appended.
 */
Document::Document() noexcept : bson_(bson_new()) {}

/**
 * @brief Constructs a `Document` by adopting and taking ownership of a raw `bson_t` pointer.
 * @details This constructor is designed for interoperability with `libbson` functions that return
 * owning `bson_t*` pointers. The `Document` instance immediately assumes responsibility for the
 * lifetime of the provided BSON object.
 * @param b An owning pointer to a heap-allocated `bson_t`. This `Document` will call `bson_destroy`
 * on it.
 */
Document::Document(bson_t *b) noexcept : bson_(b) {}

/**
 * @brief Destructor that deallocates the managed BSON document.
 * @details This is the core of the RAII implementation. If the internal `bson_` pointer is valid
 * (not null), `bson_destroy` is called to free all associated memory. The pointer is then nullified
 * to prevent dangling pointer issues.
 */
Document::~Document() {
    if (bson_) {
        bson_destroy(bson_);
        bson_ = nullptr;
    }
}

/**
 * @brief Copy constructor that performs a deep copy of another `Document`.
 * @details This method creates a new, independent `Document` by invoking `bson_copy()`. This
 * function from `libbson` allocates new memory and performs a byte-for-byte duplication of the
 * source document's serialized data, ensuring no shared state between the two `Document` objects.
 * @param other The constant reference to the `Document` to be copied.
 */
Document::Document(const Document &other) { bson_ = bson_copy(other.bson_); }

/**
 * @brief Copy assignment operator that replaces the current document with a deep copy of another.
 * @details This operator ensures safe assignment by first checking for self-assignment to prevent
 * unnecessary work and logical errors. If the current `Document` already manages data, that data
 * is destroyed via `bson_destroy`. Subsequently, a new deep copy of the `other` document is created
 * using `bson_copy`, and the internal pointer is updated to manage the new copy.
 * @param other The constant reference to the `Document` from which to copy.
 * @return A reference to the modified `Document` (`*this`).
 */
Document &Document::operator=(const Document &other) & {
    if (this != &other) {
        if (bson_) {
            bson_destroy(bson_);
        }
        bson_ = bson_copy(other.bson_);
    }
    return *this;
}

/**
 * @brief Move constructor that transfers ownership of the underlying `bson_t` from another
 * `Document`.
 * @details This operation provides an efficient, no-copy mechanism for transferring a BSON
 * document. It uses `std::exchange` to atomically take the `bson_` pointer from the `other`
 * `Document` and replace it with `nullptr`. This leaves the `other` `Document` in a valid but empty
 * state, correctly transferring ownership and preventing any double-free errors.
 * @param other The rvalue-reference to the `Document` from which the resource will be moved.
 */
Document::Document(Document &&other) noexcept { std::swap(bson_, other.bson_); }

/**
 * @brief Move assignment operator that transfers ownership of the underlying `bson_t`.
 * @details This operator provides safe and efficient resource transfer. It first checks for
 * self-assignment. If different, it deallocates its currently managed `bson_t` (if any) and then
 * takes ownership of the `other` document's resource using `std::exchange`. The `other` document
 * is subsequently nullified.
 * @param other The rvalue-reference to the `Document` from which the resource will be moved.
 * @return A reference to the modified `Document` (`*this`).
 */
Document &Document::operator=(Document &&other) & noexcept {
    if (this != &other) {
        if (bson_) {
            bson_destroy(bson_);
        }
        bson_ = std::exchange(other.bson_, nullptr);
    }
    return *this;
}

/**
 * @brief Retrieves the total size of the serialized BSON document in bytes.
 * @details This function safely accesses the `len` field of the internal `bson_t` structure. If
 * the internal pointer `bson_` is null, it correctly returns 0. Otherwise, it returns the stored
 * length, which represents the complete size of the document in its binary form.
 * @return The length of the BSON data as a `uint32_t`, or 0 if the document is null.
 */
uint32_t Document::length() const noexcept { return bson_ ? bson_->len : 0; }

/**
 * @brief Checks if the BSON document is logically empty (contains no elements).
 * @details According to the BSON specification, a valid but empty document consists of a 4-byte
 * length field (value = 5) followed by a single null byte terminator. This function efficiently
 * checks this condition by comparing the document's length.
 * @return `true` if the document contains no key-value elements, `false` otherwise.
 */
bool Document::empty() const noexcept { return length() <= 5; }

/**
 * @brief Releases ownership of the underlying `bson_t` and returns it.
 * @details After calling this, the `Document` object is left in a null state and
 *          is no longer responsible for destroying the BSON data. The caller
 *          takes full ownership and must eventually call `bson_destroy`.
 * @return The raw pointer to the `bson_t` object.
 */
bson_t *Document::release() noexcept { return std::exchange(bson_, nullptr); }

}  // namespace aevum::bson::doc
