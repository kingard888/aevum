// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file builder.hpp
 * @brief Defines the `Builder` class, a high-level utility for fluently and safely constructing
 * BSON documents.
 * @details This header file provides the interface for a `Builder` class, which offers a
 * method-chaining (fluent) API for programmatically building complex BSON documents. It abstracts
 * away the raw C-style `bson_append_*` functions from `libbson`, providing a type-safe, modern C++
 * alternative that integrates seamlessly with the `aevum::bson::doc::Document` RAII wrapper.
 */
#pragma once

#include <bson/bson.h>
#include <string_view>

#include "aevum/bson/doc/document.hpp"

/**
 * @namespace aevum::bson
 * @brief The primary namespace for all BSON-related utilities and classes within the AevumDB
 * project.
 */
namespace aevum::bson {

/**
 * @class Builder
 * @brief A fluent interface for the programmatic construction of BSON `Document` objects.
 *
 * @details The `Builder` class simplifies the process of creating BSON documents by providing a
 * chainable, type-safe set of `append` methods. It internally manages a `bson_t` structure,
 * handling its initialization and the appending of various data types. This approach prevents
 * common errors associated with using the raw `libbson` C API and results in more readable and
 * maintainable code.
 *
 * The typical usage pattern involves creating a `Builder` instance, chaining multiple `append_*`
 * calls to add key-value pairs, and finally calling `finalize()` to produce a complete,
 * memory-managed `doc::Document` object.
 *
 * @b Example
 * @code
 *   aevum::bson::Builder builder;
 *   builder.append_string("name", "John Doe")
 *          .append_int32("age", 30)
 *          .append_bool("is_active", true);
 *   aevum::bson::doc::Document user_doc = builder.finalize();
 * @endcode
 */
class Builder {
  public:
    /**
     * @brief Constructs a new `Builder` and initializes an empty BSON document.
     * @details This constructor allocates a new `bson_t` on the heap, which will be populated
     * by subsequent `append_*` calls.
     */
    Builder();

    /**
     * @brief Appends a UTF-8 string value to the document.
     * @param key The key for the element, as a null-terminated C-style string.
     * @param value The string value to append.
     * @return A reference to `*this`, allowing for method chaining.
     */
    Builder &append_string(const char *key, std::string_view value);

    /**
     * @brief Appends a 32-bit signed integer value to the document.
     * @param key The key for the element.
     * @param value The `int32_t` value to append.
     * @return A reference to `*this` for chaining.
     */
    Builder &append_int32(const char *key, int32_t value);

    /**
     * @brief Appends a 64-bit signed integer value to the document.
     * @param key The key for the element.
     * @param value The `int64_t` value to append.
     * @return A reference to `*this` for chaining.
     */
    Builder &append_int64(const char *key, int64_t value);

    /**
     * @brief Appends a double-precision floating-point value to the document.
     * @param key The key for the element.
     * @param value The `double` value to append.
     * @return A reference to `*this` for chaining.
     */
    Builder &append_double(const char *key, double value);

    /**
     * @brief Appends a boolean value to the document.
     * @param key The key for the element.
     * @param value The `bool` value to append.
     * @return A reference to `*this` for chaining.
     */
    Builder &append_bool(const char *key, bool value);

    /**
     * @brief Appends a null value to the document.
     * @param key The key for the element.
     * @return A reference to `*this` for chaining.
     */
    Builder &append_null(const char *key);

    /**
     * @brief Appends a nested BSON document to the current document.
     * @param key The key for the sub-document element.
     * @param sub_document The `doc::Document` to embed.
     * @return A reference to `*this` for chaining.
     */
    Builder &append_document(const char *key, const doc::Document &sub_document);

    /**
     * @brief Consumes the builder and returns a fully constructed, memory-managed `doc::Document`.
     * @details This method finalizes the document construction process. It transfers ownership of
     * the internal `bson_t` object to a new `doc::Document` instance, leaving the `Builder` in an
     * empty/invalid state. This `Builder` instance should not be used after `finalize()` is called.
     * @return An `aevum::bson::doc::Document` containing the built BSON data.
     */
    [[nodiscard]] doc::Document finalize();

  private:
    /**
     * @var document_
     * @brief An RAII wrapper for the `bson_t` being constructed. This ensures that even if the
     * builder is destroyed before `finalize()` is called, the underlying BSON data is safely
     * deallocated.
     */
    doc::Document document_;
};

}  // namespace aevum::bson
