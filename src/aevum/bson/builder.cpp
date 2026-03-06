// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file builder.cpp
 * @brief Implements the `Builder` class for fluently constructing BSON documents.
 * @details This source file provides the concrete definitions for the `Builder` class methods.
 * Each `append_*` method is a thin, type-safe wrapper around the corresponding C-style macro
 * from the `libbson` library, facilitating a robust and modern C++ interface.
 */
#include "aevum/bson/builder.hpp"

#include <utility>

namespace aevum::bson {

/**
 * @brief Constructs a `Builder` instance, initializing an empty underlying BSON document.
 * @details The constructor for the `Builder` implicitly calls the default constructor of its
 * `doc::Document` member, which allocates a new, empty `bson_t` structure on the heap. This
 * prepares the builder to immediately start appending elements.
 */
Builder::Builder() = default;

/**
 * @brief Appends a UTF-8 encoded string to the BSON document.
 * @details This method leverages the `BSON_APPEND_UTF8` macro from `libbson` to append a
 * key-value pair where the value is a string. It passes the key and the string data directly
 * to the underlying C library.
 * @param key The null-terminated string to be used as the key.
 * @param value A `std::string_view` representing the string value.
 * @return A reference to `*this` to enable fluent method chaining.
 */
Builder &Builder::append_string(const char *key, std::string_view value) {
    BSON_APPEND_UTF8(document_.get(), key, value.data());
    return *this;
}

/**
 * @brief Appends a 32-bit signed integer to the BSON document.
 * @param key The null-terminated string to be used as the key.
 * @param value The `int32_t` integer value.
 * @return A reference to `*this` to enable fluent method chaining.
 */
Builder &Builder::append_int32(const char *key, int32_t value) {
    BSON_APPEND_INT32(document_.get(), key, value);
    return *this;
}

/**
 * @brief Appends a 64-bit signed integer to the BSON document.
 * @param key The null-terminated string to be used as the key.
 * @param value The `int64_t` integer value.
 * @return A reference to `*this` to enable fluent method chaining.
 */
Builder &Builder::append_int64(const char *key, int64_t value) {
    BSON_APPEND_INT64(document_.get(), key, value);
    return *this;
}

/**
 * @brief Appends a double-precision floating-point number to the BSON document.
 * @param key The null-terminated string to be used as the key.
 * @param value The `double` value.
 * @return A reference to `*this` to enable fluent method chaining.
 */
Builder &Builder::append_double(const char *key, double value) {
    BSON_APPEND_DOUBLE(document_.get(), key, value);
    return *this;
}

/**
 * @brief Appends a boolean value to the BSON document.
 * @param key The null-terminated string to be used as the key.
 * @param value The `bool` value (`true` or `false`).
 * @return A reference to `*this` to enable fluent method chaining.
 */
Builder &Builder::append_bool(const char *key, bool value) {
    BSON_APPEND_BOOL(document_.get(), key, value);
    return *this;
}

/**
 * @brief Appends a BSON null value to the document.
 * @param key The null-terminated string to be used as the key.
 * @return A reference to `*this` to enable fluent method chaining.
 */
Builder &Builder::append_null(const char *key) {
    BSON_APPEND_NULL(document_.get(), key);
    return *this;
}

/**
 * @brief Appends a nested sub-document to the BSON document.
 * @details This method embeds a copy of the provided `sub_document` into the document being built.
 * It uses the `BSON_APPEND_DOCUMENT` macro, which handles the serialization of the sub-document's
 * data into the correct position within the parent document.
 * @param key The null-terminated string to be used as the key for the sub-document.
 * @param sub_document A constant reference to the `doc::Document` to be embedded.
 * @return A reference to `*this` to enable fluent method chaining.
 */
Builder &Builder::append_document(const char *key, const doc::Document &sub_document) {
    BSON_APPEND_DOCUMENT(document_.get(), key, sub_document.get());
    return *this;
}

/**
 * @brief Finalizes the document and transfers ownership of the underlying BSON data.
 * @details This is the terminal operation for a `Builder`. It atomically transfers the pointer to
 * the constructed `bson_t` from this `Builder` to the returned `doc::Document`, replacing the
 * internal pointer with `nullptr`. This leaves the `Builder` in an empty, safe-to-destruct state
 * and ensures the returned `doc::Document` is the sole owner of the resource.
 * @return A `doc::Document` instance containing the finished BSON document.
 */
doc::Document Builder::finalize() { return doc::Document(document_.release()); }

}  // namespace aevum::bson
