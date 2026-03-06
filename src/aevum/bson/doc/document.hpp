// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file document.hpp
 * @brief Defines the `Document` class, a C++ RAII wrapper that provides robust, memory-safe
 * management for the underlying `bson_t` type from libbson.
 * @details This header is foundational to BSON manipulation within the AevumDB ecosystem. It
 * declares the `Document` class, which encapsulates a raw `bson_t*` and automates its lifecycle
 * management. By adhering to the Resource Acquisition Is Initialization (RAII) idiom, it
 * eliminates common memory leaks associated with manual `bson_destroy` calls and provides a
 * modern C++ interface with full support for copy, move, and other idiomatic operations.
 */
#pragma once

#include <bson/bson.h>
#include <cstdint>

/**
 * @namespace aevum::bson::doc
 * @brief Encapsulates core structures and utilities for representing and managing BSON documents.
 * @details This namespace provides the foundational C++ abstractions over the C-based libbson
 * library, focusing on safety, correctness, and ease of use.
 */
namespace aevum::bson::doc {

/**
 * @class Document
 * @brief A high-level C++ abstraction that manages the memory and lifecycle of a `bson_t` object.
 *
 * @details This class is a cornerstone of safe BSON manipulation. It serves as a resource handle
 * for a heap-allocated `bson_t` structure from the `libbson` C library. By leveraging the RAII
 * (Resource Acquisition Is Initialization) principle, it guarantees that the underlying BSON
 * data is deterministically and automatically deallocated via `bson_destroy` when a `Document`
 * instance goes out of scope. This design eradicates a major class of memory leaks.
 *
 * The class provides a complete, modern C++ interface, including deep-copy semantics for copy
 * construction and assignment, and efficient, ownership-transfer semantics for move construction
 * and assignment. This allows `Document` objects to be used seamlessly and safely within standard
 * C++ containers and algorithms.
 */
class Document {
  public:
    /**
     * @brief Constructs a new, empty, but valid BSON document.
     * @details This constructor allocates a fresh `bson_t` structure on the heap via `bson_new()`.
     * The resulting document is initialized and ready to have elements appended to it. This
     * operation is guaranteed not to throw exceptions (`noexcept`).
     */
    Document() noexcept;

    /**
     * @brief Constructs a `Document` by taking exclusive ownership of a pre-existing `bson_t`.
     * @warning The caller relinquishes ownership of the `b` pointer. The provided pointer must
     *          point to a valid, heap-allocated `bson_t` object (e.g., from `bson_new()` or
     *          `bson_new_from_json()`), as this `Document` instance will take responsibility
     *          for calling `bson_destroy` on it. Providing a stack-allocated or invalid pointer
     *          will result in undefined behavior.
     * @param b A raw pointer to a `bson_t` whose lifecycle this `Document` will now manage.
     */
    explicit Document(bson_t *b) noexcept;

    /**
     * @brief Destructor that ensures the automatic destruction and deallocation of the managed
     * `bson_t`.
     * @details If the internal `bson_` pointer is not null, this destructor invokes `bson_destroy`,
     * which safely frees all memory associated with the BSON document. This is the core of the
     * RAII guarantee.
     */
    ~Document();

    /**
     * @brief Copy constructor. Performs a deep, byte-for-byte copy of the BSON data.
     * @details This constructor creates a completely independent new `Document` by calling
     * `bson_copy()`, which allocates new memory and duplicates the entire content of the `other`
     * document.
     * @param other The source `Document` from which to create a copy.
     */
    Document(const Document &other);

    /**
     * @brief Copy assignment operator. Replaces this document with a deep copy of another.
     * @details This operator provides safe assignment by first checking for self-assignment. If
     * this `Document` already manages a `bson_t`, that memory is safely destroyed before a deep
     * copy of the `other` document's data is created and assigned. The lvalue-ref-qualifier `&`
     * prevents assignment to rvalues.
     * @param other The source `Document` to copy from.
     * @return A reference to this instance (`*this`) after the copy.
     */
    Document &operator=(const Document &other) &;

    /**
     * @brief Move constructor. Efficiently transfers ownership of the BSON data from another
     * `Document`.
     * @details This is a highly efficient, no-copy operation that "steals" the internal `bson_t`
     * pointer from the `other` document. The `other` document is subsequently left in a valid but
     * null (empty) state to prevent double-free errors.
     * @param other The rvalue-reference to the `Document` to move from.
     */
    Document(Document &&other) noexcept;

    /**
     * @brief Move assignment operator. Efficiently transfers ownership from another `Document`.
     * @details This operator first safely destroys any BSON data it currently manages. It then
     * proceeds to transfer ownership of the `other` document's `bson_t` pointer. The `other`
     * document is left in a null state. This is a fast, no-copy operation. The lvalue-ref-qualifier
     * `&` prevents assignment to rvalues.
     * @param other The rvalue-reference to the `Document` to move from.
     * @return A reference to this instance (`*this`) after the move.
     */
    Document &operator=(Document &&other) & noexcept;

    /**
     * @brief Retrieves a mutable raw pointer to the underlying `bson_t` structure.
     * @details This function provides an escape hatch for interoperability with C-style `libbson`
     * functions that require a non-const `bson_t*` (e.g., for appending elements). The caller
     * should not deallocate this pointer, as its lifetime is still managed by the `Document` class.
     * @return A raw pointer to the internal `bson_t`, or `nullptr` if the document is in a null
     * state.
     */
    [[nodiscard]] bson_t *get() noexcept { return bson_; }

    /**
     * @brief Retrieves a read-only raw pointer to the underlying `bson_t` structure.
     * @details This accessor is essential for interoperating with `libbson` functions that perform
     * read-only operations and thus accept a `const bson_t*`.
     * @return A `const` raw pointer to the internal `bson_t`, or `nullptr` if the document is null.
     */
    [[nodiscard]] const bson_t *get() const noexcept { return bson_; }

    /**
     * @brief Determines if the document contains any key-value pairs.
     * @details A BSON document is considered empty if its serialized length is exactly 5 bytes.
     * This size accounts for the 4-byte integer representing the total length (which is 5) and the
     * trailing null byte that terminates the document structure.
     * @return `true` if the document has no elements, `false` otherwise.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Gets the total size of the fully serialized BSON data in bytes.
     * @details This value includes the initial 4-byte length field, all key-value elements, and the
     * final null terminator. It represents the exact amount of memory the document occupies on disk
     * or when sent over a network.
     * @return The length of the document in bytes as a 32-bit unsigned integer.
     */
    [[nodiscard]] uint32_t length() const noexcept;

    /**
     * @brief Releases ownership of the underlying `bson_t` and returns it.
     * @details After calling this, the `Document` object is left in a null state and
     *          is no longer responsible for destroying the BSON data. The caller
     *          takes full ownership and must eventually call `bson_destroy`.
     * @return The raw pointer to the `bson_t` object.
     */
    [[nodiscard]] bson_t *release() noexcept;

  private:
    /**
     * @var bson_
     * @brief A raw pointer to the heap-allocated `bson_t` object whose lifetime is exclusively
     * managed by this `Document` instance. It is initialized to `nullptr`.
     */
    bson_t *bson_{nullptr};
};

}  // namespace aevum::bson::doc
