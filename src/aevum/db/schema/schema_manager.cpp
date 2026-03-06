// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file schema_manager.cpp
 * @brief Implements the `SchemaManager` class for managing schema validation, caching, and
 * persistence.
 * @details This file provides the concrete implementations for the methods declared in
 * `schema_manager.hpp`. It orchestrates the interaction between the in-memory schema cache,
 * the persistent `WiredTigerStore`, and the Rust-based validation engine.
 */
#include "aevum/db/schema/schema_manager.hpp"

#include <mutex>
#include <shared_mutex>

#include "aevum/db/schema/validator.hpp"
#include "aevum/util/log/logger.hpp"

namespace aevum::db::schema {

/**
 * @brief Constructs a `SchemaManager` instance, linking it with a persistent storage backend.
 * @param storage A reference to the `WiredTigerStore` which will be used for all schema
 *        persistence operations.
 */
SchemaManager::SchemaManager(aevum::db::storage::WiredTigerStore &storage) : storage_(storage) {}

/**
 * @brief Validates a BSON document against the schema registered for its collection.
 * @details This function implements a highly concurrent validation check. It acquires a shared
 * (read) lock, allowing multiple validations to occur in parallel. It first searches the in-memory
 * `schemas_` cache for a schema corresponding to the given `collection`.
 *
 * If no schema is found, the function returns `Status::OK()` immediately, as the absence of a
 * schema implies that any document is valid for that collection.
 *
 * If a schema is found, validation is delegated to the `validate_via_rust` function, which
 * handles the FFI call to the high-performance Rust validation core. A failure from the Rust
 * core results in a logged warning and an `InvalidArgument` status return.
 *
 * @param collection A `std::string_view` identifying the name of the collection.
 * @param doc The `aevum::bson::doc::Document` to be validated.
 * @return `aevum::util::Status::OK()` if the document is valid or if no schema exists for the
 *         collection.
 * @return `aevum::util::Status::InvalidArgument` if the document fails validation against the
 *         collection's schema.
 */
aevum::util::Status SchemaManager::validate(std::string_view collection,
                                            const aevum::bson::doc::Document &doc) const {
    std::string coll_str(collection);
    aevum::util::log::Logger::debug("SchemaManager: Performing schema validation for collection '" +
                                    coll_str + "'.");
    // Acquire a shared (read) lock to enable concurrent schema validation checks.
    std::shared_lock<std::shared_mutex> lock(schemas_lock_);

    auto it = schemas_.find(coll_str);

    // If no schema is registered for this collection, validation trivially passes.
    if (it == schemas_.end()) {
        aevum::util::log::Logger::debug("SchemaManager: No schema found for collection '" +
                                        coll_str + "'. Validation skipped (OK).");
        return aevum::util::Status::OK();
    }

    // Delegate the actual validation logic to the high-performance Rust FFI bridge.
    bool is_valid = validate_via_rust(doc, it->second);

    if (!is_valid) {
        aevum::util::log::Logger::warn(
            "SchemaManager: Document failed schema validation for collection '" + coll_str + "'.");
        return aevum::util::Status::InvalidArgument(
            "Document does not conform to the collection's schema.");
    }

    aevum::util::log::Logger::debug(
        "SchemaManager: Document successfully validated against schema for collection '" +
        coll_str + "'.");
    return aevum::util::Status::OK();
}

/**
 * @brief Persists a new or updated schema for a collection and updates the in-memory cache.
 * @details This function orchestrates the two-phase process of updating a schema. First, it
 * constructs a new BSON document (`persistent_schema`) that embeds the collection name within
 * the schema document itself, which is the format required for storage in the `_schemas` system
 * collection. It then calls `storage_.put()` to durably write this schema to WiredTiger.
 *
 * Upon successful persistence, the function proceeds to update the in-memory `schemas_` cache
 * by calling `add_schema_to_cache` with the original, unmodified schema document. This ensures
 * that the in-memory representation is consistent with the newly persisted state.
 *
 * @param collection A `std::string_view` of the collection name.
 * @param schema The `aevum::bson::doc::Document` defining the new schema rules.
 * @return `aevum::util::Status::OK()` on complete success. If the storage `put` operation fails,
 *         the corresponding error status is returned, and the in-memory cache is not updated.
 */
aevum::util::Status SchemaManager::set_schema(std::string_view collection,
                                              const aevum::bson::doc::Document &schema) {
    std::string coll_str(collection);
    aevum::util::log::Logger::debug("SchemaManager: Setting schema for collection '" + coll_str +
                                    "'.");

    // Transform simple "field": "type" schemas into "field": {"$type": "type"}
    // to correctly leverage the matches_query logic in the Rust core.
    bson_t *transformed = bson_new();
    bson_iter_t iter;
    if (bson_iter_init(&iter, schema.get())) {
        while (bson_iter_next(&iter)) {
            const char *key = bson_iter_key(&iter);
            if (BSON_ITER_HOLDS_UTF8(&iter)) {
                const char *type_val = bson_iter_utf8(&iter, nullptr);
                // If it looks like a simple type string, wrap it in a $type operator.
                bson_t child;
                bson_append_document_begin(transformed, key, -1, &child);
                BSON_APPEND_UTF8(&child, "$type", type_val);
                bson_append_document_end(transformed, &child);
            } else {
                // If it's already an object or other type, copy it as-is.
                bson_append_value(transformed, key, -1, bson_iter_value(&iter));
            }
        }
    }
    aevum::bson::doc::Document final_schema(transformed);

    // To store the schema, we create a meta-document that includes the collection name it applies
    // to.
    bson_t *b = bson_new();
    bson_append_utf8(b, "collection", -1, coll_str.c_str(), -1);
    bson_concat(b, final_schema.get());
    aevum::bson::doc::Document persistent_schema(b);

    // Persist the schema to the dedicated `_schemas` system collection.
    auto status = storage_.put("_schemas", coll_str, persistent_schema);
    if (!status.ok()) {
        aevum::util::log::Logger::error("SchemaManager: Failed to persist schema for collection '" +
                                        coll_str + "'. Status: " + status.to_string());
        return status;
    }

    // After successful persistence, update the in-memory cache with the new schema.
    add_schema_to_cache(coll_str, std::move(final_schema));

    aevum::util::log::Logger::info(
        "SchemaManager: Successfully set and persisted schema for collection '" + coll_str + "'.");
    return aevum::util::Status::OK();
}

/**
 * @brief Retrieves a copy of the schema for a collection from the in-memory cache.
 * @details This method performs a read-locked lookup in the `schemas_` map.
 * @param collection The name of the collection.
 * @return An optional containing the schema document if found.
 */
std::optional<aevum::bson::doc::Document> SchemaManager::get_schema(
    std::string_view collection) const {
    std::shared_lock<std::shared_mutex> lock(schemas_lock_);
    auto it = schemas_.find(std::string(collection));
    if (it != schemas_.end()) {
        return it->second;
    }
    return std::nullopt;
}

/**
 * @brief Atomically adds a schema to the in-memory cache.
 * @details This function is designed for populating the `schemas_` map in a thread-safe manner.
 * It acquires a `std::unique_lock`, providing exclusive write access to the cache. This blocks
 * all other read and write operations until the insertion is complete, preventing data races.
 * It uses `insert_or_assign` with `std::move` to efficiently transfer ownership of the collection
 * name and schema document into the map, minimizing copies.
 *
 * @param collection The collection name `std::string`, which will be moved.
 * @param schema The `aevum::bson::doc::Document` schema, which will be moved.
 */
void SchemaManager::add_schema_to_cache(std::string collection, aevum::bson::doc::Document schema) {
    aevum::util::log::Logger::debug("SchemaManager: Caching schema for collection '" + collection +
                                    "'.");
    // Acquire an exclusive write lock to ensure the atomicity of the cache update.
    std::unique_lock<std::shared_mutex> lock(schemas_lock_);

    // Use insert_or_assign for efficient insertion or update, moving the resources into the map.
    schemas_.insert_or_assign(std::move(collection), std::move(schema));
}

}  // namespace aevum::db::schema
