// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file core.cpp
 * @brief Implements the `Core` class, the central orchestrating component of the AevumDB engine.
 * @details This file provides the concrete implementations for the methods of the `Core` class. It
 * handles the initialization sequence, the routing of high-level API calls to the appropriate
 * subsystems (storage, indexing, auth, schema), and the complex interplay with the Rust FFI for
 * query execution.
 */
#include "aevum/db/core/core.hpp"

#include <bson/bson.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>

#include "aevum/bson/json/parser.hpp"
#include "aevum/bson/json/serializer.hpp"
#include "aevum/db/ffi.hpp"
#include "aevum/util/hash/djb2.hpp"
#include "aevum/util/log/logger.hpp"
#include "aevum/util/uuid/v4.hpp"

namespace aevum::db {

/**
 * @brief Constructs and initializes the `Core` database engine.
 * @details The constructor executes the critical startup sequence for the entire database. It
 * initializes the member subsystems in a specific order: `storage_`, `auth_manager_`,
 * `schema_manager_`, and `index_manager_`. It then invokes `storage_.init()` to prepare the
 * physical storage layer. A critical failure at this stage is considered fatal and will terminate
 * the application.
 *
 * Following storage initialization, it calls `load_all()` to populate the in-memory caches and
 * indexes from persisted data. Finally, it performs a security bootstrap check: if the
 * `auth_manager_` is empty after loading, it creates a default 'root' administrator user to
 * ensure the database is not left in an inaccessible state.
 *
 * @param data_dir The filesystem path that will be used by the `WiredTigerStore` for all
 *        database files.
 */
Core::Core(std::string data_dir)
    : storage_(std::move(data_dir)),
      auth_manager_(),
      schema_manager_(storage_),
      index_manager_(storage_) {
    aevum::util::log::Logger::info("Core: Initializing database engine...");
    aevum::util::log::Logger::debug("Core: Data directory set to '" + data_dir + "'.");

    auto status = storage_.init();
    if (!status.ok()) {
        aevum::util::log::Logger::fatal(
            "Core: Failed to initialize persistence layer (WiredTiger). Status: " +
            status.to_string());
        std::abort();  // A failure to initialize storage is a non-recoverable, fatal error.
    }

    load_all();

    // If the authentication database is empty, bootstrap a default admin user.
    if (auth_manager_.empty()) {
        aevum::util::log::Logger::warn(
            "Core: Security credentials not found. Bootstrapping default 'root' admin user.");
        create_user("root", auth::UserRole::ADMIN);
    }

    aevum::util::log::Logger::info("Core: Database engine is online and ready for operations.");
}

/**
 * @brief Destroys the `Core` engine, ensuring a graceful shutdown.
 */
Core::~Core() { aevum::util::log::Logger::info("Core: Shutting down database engine."); }

/**
 * @brief Loads all system and user data from the persistent `WiredTigerStore` into the
 * appropriate in-memory managers.
 * @details This function is a critical part of the startup process. It lists all collections
 * (tables) in the database and processes each one:
 * - `_indexes`: Loads secondary index definitions into the `IndexManager`.
 * - `_schemas`: Loads schema definitions into the `SchemaManager`.
 * - `_auth`: Loads user credentials into the `AuthManager`.
 * - All other collections are treated as user data collections, and `index_manager_.rebuild_index`
 *   is called to populate their primary and secondary indexes in memory.
 */
void Core::load_all() {
    aevum::util::log::Logger::info("Core: Starting data loading sequence from persistence layer.");
    auto collections = storage_.list_collections();
    aevum::util::log::Logger::debug("Core: Discovered " + std::to_string(collections.size()) +
                                    " collections in storage.");

    for (const auto &name : collections) {
        if (name == "_indexes") {
            aevum::util::log::Logger::debug("Core: Loading system collection '_indexes'.");
            auto index_status = index_manager_.load_all_index_definitions();
            if (!index_status.ok()) {
                aevum::util::log::Logger::warn("Core: Failed to load index definitions. Status: " +
                                               index_status.to_string());
            }
            continue;
        }

        aevum::util::log::Logger::debug("Core: Loading data for collection '" + name + "'.");
        std::vector<aevum::bson::doc::Document> docs = storage_.load_collection(name);
        aevum::util::log::Logger::debug("Core: Loaded " + std::to_string(docs.size()) +
                                        " documents from collection '" + name + "'.");

        if (name == "_schemas") {
            aevum::util::log::Logger::debug("Core: Processing system collection '_schemas'.");
            for (const auto &doc : docs) {
                bson_iter_t iter;
                if (bson_iter_init_find(&iter, doc.get(), "collection") &&
                    BSON_ITER_HOLDS_UTF8(&iter)) {
                    std::string coll_name = bson_iter_utf8(&iter, nullptr);
                    schema_manager_.add_schema_to_cache(coll_name, aevum::bson::doc::Document(doc));
                }
            }
            continue;
        }

        if (name == "_auth") {
            aevum::util::log::Logger::debug("Core: Processing system collection '_auth'.");
            for (const auto &doc : docs) {
                bson_iter_t iter;
                std::string key_hash;
                auth::UserRole role = auth::UserRole::READ_ONLY;

                if (bson_iter_init_find(&iter, doc.get(), "key_hash") &&
                    BSON_ITER_HOLDS_UTF8(&iter)) {
                    key_hash = bson_iter_utf8(&iter, nullptr);
                }
                if (bson_iter_init_find(&iter, doc.get(), "role") && BSON_ITER_HOLDS_UTF8(&iter)) {
                    std::string role_str = bson_iter_utf8(&iter, nullptr);
                    if (role_str == "ADMIN")
                        role = auth::UserRole::ADMIN;
                    else if (role_str == "READ_WRITE")
                        role = auth::UserRole::READ_WRITE;
                }

                if (!key_hash.empty()) {
                    auth_manager_.add_user(key_hash, role);
                }
            }
            aevum::util::log::Logger::info(
                "Core: Security policies and user roles loaded into cache.");
            continue;
        }

        // For all non-system collections, rebuild their indexes from the loaded documents.
        aevum::util::log::Logger::info("Core: Rebuilding indexes for user collection '" + name +
                                       "'.");
        index_manager_.rebuild_index(name, docs);
    }
    aevum::util::log::Logger::info("Core: Data loading sequence complete.");
}

/**
 * @brief A helper function to fetch an entire collection from storage and serialize it to a JSON
 * array string.
 * @details This utility is primarily used to prepare data for the Rust FFI functions, which expect
 * datasets to be passed as a single JSON string. It loads all documents from the specified
 * collection and concatenates their JSON representations into a single string.
 * @param coll The name of the collection to fetch.
 * @return A `std::string` containing the JSON array of all documents.
 */
std::string Core::fetch_collection_as_json_array(std::string_view coll) {
    aevum::util::log::Logger::debug("Core: Fetching all documents from collection '" +
                                    std::string(coll) + "' from in-memory index for FFI.");
    std::vector<aevum::bson::doc::Document> docs = index_manager_.get_all_documents(coll);

    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < docs.size(); ++i) {
        oss << aevum::bson::json::to_string(docs[i]);
        if (i < docs.size() - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

/**
 * @brief Inserts a single document into a collection, ensuring data integrity and indexing.
 * @details This is a write-locked operation. The workflow is as follows:
 * 1. An exclusive lock is acquired on the `rw_lock_`.
 * 2. It checks for a pre-existing `_id`. If one is not found, a new UUIDv4 is generated and
 *    prepended to the document.
 * 3. The document is validated against the collection's schema via the `SchemaManager`.
 * 4. The document is persisted to the `WiredTigerStore`.
 * 5. The document is added to the primary and all relevant secondary indexes via the
 * `IndexManager`.
 *
 * @param coll The name of the target collection.
 * @param doc The `aevum::bson::doc::Document` to insert.
 * @return A pair containing the operation status and the document's final `_id` string.
 */
std::pair<aevum::util::Status, std::string> Core::insert(std::string_view coll,
                                                         aevum::bson::doc::Document doc) {
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    aevum::util::log::Logger::debug("Core: Beginning insert operation for collection '" +
                                    std::string(coll) + "'.");

    std::string id_str;
    bson_iter_t iter;

    if (bson_iter_init_find(&iter, doc.get(), "_id") && BSON_ITER_HOLDS_UTF8(&iter)) {
        id_str = bson_iter_utf8(&iter, nullptr);
        aevum::util::log::Logger::debug("Core: Using provided _id '" + id_str + "' for insert.");
    } else {
        id_str = aevum::util::uuid::generate_v4();
        aevum::util::log::Logger::debug("Core: No _id provided. Generated new UUID '" + id_str +
                                        "' for insert.");

        bson_t *new_b = bson_new();
        BSON_APPEND_UTF8(new_b, "_id", id_str.c_str());
        bson_concat(new_b, doc.get());
        doc = aevum::bson::doc::Document(new_b);
    }

    auto validation_status = schema_manager_.validate(coll, doc);
    if (!validation_status.ok()) {
        aevum::util::log::Logger::warn(
            "Core: Insert failed for collection '" + std::string(coll) +
            "' due to schema validation failure. Status: " + validation_status.to_string());
        return {validation_status, ""};
    }

    auto status = storage_.put(coll, id_str, doc);
    if (!status.ok()) {
        aevum::util::log::Logger::error(
            "Core: Insert failed for collection '" + std::string(coll) +
            "' during storage persistence. Status: " + status.to_string());
        return {status, ""};
    }

    index_manager_.add_document_to_indexes(coll, doc);
    aevum::util::log::Logger::info("Core: Successfully inserted document '" + id_str +
                                   "' into collection '" + std::string(coll) + "'.");
    return {aevum::util::Status::OK(), id_str};
}

/**
 * @brief Performs an "upsert": updates a document if it exists, otherwise inserts it.
 * @param coll The target collection name.
 * @param query_json A JSON query to find the document to upsert.
 * @param doc The document data to insert or use for updating.
 * @return `aevum::util::Status::OK()` on success, or an error status on failure.
 */
aevum::util::Status Core::upsert(std::string_view coll, std::string_view query_json,
                                 const aevum::bson::doc::Document &doc) {
    aevum::util::log::Logger::debug("Core: Beginning upsert operation for collection '" +
                                    std::string(coll) + "'.");
    if (count(coll, query_json) > 0) {
        aevum::util::log::Logger::debug(
            "Core: Upsert found matching document(s). Proceeding with update path.");
        std::string update_json = aevum::bson::json::to_string(doc);
        return update(coll, query_json, update_json).first;
    } else {
        aevum::util::log::Logger::debug(
            "Core: Upsert found no matching documents. Proceeding with insert path.");
        aevum::bson::doc::Document doc_copy(doc);
        return insert(coll, std::move(doc_copy)).first;
    }
}

/**
 * @brief Counts documents in a collection matching a query via the Rust FFI.
 * @param coll The collection to query.
 * @param query_json A JSON string representing the query criteria.
 * @return The number of matching documents.
 */
int Core::count(std::string_view coll, std::string_view query_json) {
    std::shared_lock<std::shared_mutex> lock(rw_lock_);
    std::string collection_json = fetch_collection_as_json_array(coll);
    std::string q_str(query_json);
    aevum::util::log::Logger::debug("Core: Dispatching count query to FFI for collection '" +
                                    std::string(coll) + "'.");
    return rust_count(collection_json.c_str(), q_str.c_str());
}

/**
 * @brief Finds documents in a collection, delegating complex queries to the Rust FFI.
 * @details This is a read-locked operation. It includes a critical optimization: if the query is a
 * simple lookup by `_id`, it serves the request directly from the high-speed primary index. For all
 * other queries, it serializes the entire collection to a JSON string and dispatches the find
 * operation to the Rust FFI, which handles filtering, sorting, and projection. The FFI result is
 * then parsed back into BSON documents.
 *
 * @return A vector of BSON documents that match the criteria.
 */
std::vector<aevum::bson::doc::Document> Core::find(std::string_view coll,
                                                   std::string_view query_json,
                                                   std::string_view sort_json,
                                                   std::string_view projection_json, int64_t limit,
                                                   int64_t skip) {
    std::shared_lock<std::shared_mutex> lock(rw_lock_);
    aevum::util::log::Logger::debug("Core: Beginning find operation for collection '" +
                                    std::string(coll) + "'.");

    aevum::bson::doc::Document query_doc;
    if (aevum::bson::json::parse(query_json, query_doc).ok()) {
        bson_iter_t iter;
        if (bson_iter_init(&iter, query_doc.get()) && bson_iter_next(&iter) &&
            std::string_view(bson_iter_key(&iter)) == "_id" && !bson_iter_next(&iter)) {
            bson_iter_init_find(&iter, query_doc.get(), "_id");
            if (BSON_ITER_HOLDS_UTF8(&iter)) {
                std::string id_to_find = bson_iter_utf8(&iter, nullptr);
                aevum::util::log::Logger::debug("Core: Find optimized for single _id lookup: '" +
                                                id_to_find + "'.");
                auto found_doc = index_manager_.get_document_by_id(coll, id_to_find);
                if (found_doc) {
                    return {std::move(*found_doc)};
                }
                return {};
            }
        }
    }

    aevum::util::log::Logger::debug(
        "Core: Complex find query detected. Dispatching to FFI for collection '" +
        std::string(coll) + "'.");
    std::string collection_json = fetch_collection_as_json_array(coll);

    std::string q_str(query_json);
    std::string s_str(sort_json);
    std::string p_str(projection_json);

    std::unique_ptr<char, decltype(&rust_free_string)> result_json_ptr(
        rust_find(collection_json.c_str(), q_str.c_str(), s_str.c_str(), p_str.c_str(), limit,
                  skip),
        &rust_free_string);
    std::string result_json = result_json_ptr ? result_json_ptr.get() : "[]";

    std::vector<aevum::bson::doc::Document> results;
    aevum::bson::doc::Document result_array_doc;
    if (aevum::bson::json::parse(result_json, result_array_doc).ok()) {
        bson_iter_t array_iter;
        if (bson_iter_init(&array_iter, result_array_doc.get())) {
            while (bson_iter_next(&array_iter)) {
                if (BSON_ITER_HOLDS_DOCUMENT(&array_iter)) {
                    const uint8_t *doc_data = nullptr;
                    uint32_t doc_len = 0;
                    bson_iter_document(&array_iter, &doc_len, &doc_data);

                    bson_t *b = bson_new_from_data(doc_data, doc_len);
                    if (b) results.emplace_back(b);
                }
            }
        }
    }
    aevum::util::log::Logger::debug("Core: Find operation completed, returning " +
                                    std::to_string(results.size()) + " documents.");
    return results;
}

/**
 * @brief Updates documents matching a query, synchronizing the entire collection state afterward.
 * @details This is a write-locked operation. It sends the entire collection and the update query to
 * the Rust FFI. The FFI returns a JSON string of the completely modified collection. This C++ side
 * then parses this new dataset and uses the `storage_.sync_collection` method to atomically
 * replace the old collection data in storage. Finally, the indexes are rebuilt.
 *
 * @return A pair containing the status and the number of documents in the newly synced collection.
 */
std::pair<aevum::util::Status, int> Core::update(std::string_view coll, std::string_view query_json,
                                                 std::string_view update_json) {
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    aevum::util::log::Logger::debug("Core: Beginning update operation for collection '" +
                                    std::string(coll) + "'. Dispatching to FFI.");
    std::string collection_json = fetch_collection_as_json_array(coll);

    std::string q_str(query_json);
    std::string u_str(update_json);

    // Retrieve the schema if one is set for this collection.
    std::string schema_json = "";
    auto schema_opt = schema_manager_.get_schema(coll);
    if (schema_opt) {
        schema_json = aevum::bson::json::to_string(*schema_opt);
    }

    rust_update_result res =
        rust_update(collection_json.c_str(), q_str.c_str(), u_str.c_str(), schema_json.c_str());

    std::string updated_collection_json = res.data ? res.data : "";
    int affected_count = res.modified_count;

    if (updated_collection_json.empty() || updated_collection_json == "[]") {
        rust_free_update_result(res);
        if (affected_count == 0) {
            aevum::util::log::Logger::warn("Core: Update for collection '" + std::string(coll) +
                                           "' resulted in no matches or all validation failures.");
            return {aevum::util::Status::NotFound("No documents were modified."), 0};
        }
    }

    aevum::bson::doc::Document new_collection_doc;
    if (auto status = aevum::bson::json::parse(updated_collection_json, new_collection_doc);
        !status.ok()) {
        rust_free_update_result(res);
        aevum::util::log::Logger::error(
            "Core: Failed to parse updated collection JSON received from FFI. Status: " +
            status.to_string());
        return {status, 0};
    }

    // After parsing, we don't need the raw string from Rust anymore.
    rust_free_update_result(res);

    std::vector<std::pair<std::string, aevum::bson::doc::Document>> sync_batch;
    std::vector<aevum::bson::doc::Document> index_batch;

    bson_iter_t array_iter;
    if (bson_iter_init(&array_iter, new_collection_doc.get())) {
        while (bson_iter_next(&array_iter)) {
            if (BSON_ITER_HOLDS_DOCUMENT(&array_iter)) {
                const uint8_t *doc_data = nullptr;
                uint32_t doc_len = 0;
                bson_iter_document(&array_iter, &doc_len, &doc_data);

                bson_t *b = bson_new_from_data(doc_data, doc_len);
                if (b) {
                    aevum::bson::doc::Document doc(b);
                    bson_iter_t id_iter;
                    if (bson_iter_init_find(&id_iter, doc.get(), "_id") &&
                        BSON_ITER_HOLDS_UTF8(&id_iter)) {
                        std::string id_str = bson_iter_utf8(&id_iter, nullptr);
                        sync_batch.emplace_back(id_str, aevum::bson::doc::Document(doc));
                        index_batch.push_back(std::move(doc));
                    }
                }
            }
        }
    }

    aevum::util::log::Logger::debug("Core: Synchronizing " + std::to_string(sync_batch.size()) +
                                    " documents back to storage for collection '" +
                                    std::string(coll) + "'.");
    if (auto status = storage_.sync_collection(coll, sync_batch); !status.ok()) {
        aevum::util::log::Logger::error(
            "Core: Storage synchronization failed during update for collection '" +
            std::string(coll) + "'. Status: " + status.to_string());
        return {status, 0};
    }

    aevum::util::log::Logger::debug("Core: Rebuilding indexes for collection '" +
                                    std::string(coll) + "' post-update.");
    index_manager_.rebuild_index(coll, index_batch);
    aevum::util::log::Logger::info("Core: Update operation completed for collection '" +
                                   std::string(coll) + "'. " + std::to_string(affected_count) +
                                   " documents modified.");
    return {aevum::util::Status::OK(), affected_count};
}

/**
 * @brief Removes documents from a collection that match a given query.
 * @details This write-locked operation first performs a read-only `find` via the FFI, projecting
 * only the `_id` field to efficiently gather the list of documents to be deleted. It then
 * iterates through this list of IDs, removing each document from the `WiredTigerStore` and
 * updating the `IndexManager` to remove it from all indexes.
 *
 * @return A pair containing the status and the number of documents successfully removed.
 */
std::pair<aevum::util::Status, int> Core::remove(std::string_view coll,
                                                 std::string_view query_json) {
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    aevum::util::log::Logger::debug("Core: Beginning remove operation for collection '" +
                                    std::string(coll) + "'.");
    std::string collection_json = fetch_collection_as_json_array(coll);

    std::string q_str(query_json);

    std::unique_ptr<char, decltype(&rust_free_string)> result_json_ptr(
        rust_find(collection_json.c_str(), q_str.c_str(), "{}", "{\"_id\":1}", 0, 0),
        &rust_free_string);

    std::string result_json = result_json_ptr ? result_json_ptr.get() : "[]";

    aevum::bson::doc::Document matching_docs_doc;
    if (!aevum::bson::json::parse(result_json, matching_docs_doc).ok()) {
        aevum::util::log::Logger::error(
            "Core: Failed to parse _id list from FFI during remove for collection '" +
            std::string(coll) + "'.");
        return {aevum::util::Status::NotFound("No documents found to remove."), 0};
    }

    std::vector<std::string> ids_to_remove;
    bson_iter_t array_iter;
    if (bson_iter_init(&array_iter, matching_docs_doc.get())) {
        while (bson_iter_next(&array_iter)) {
            if (BSON_ITER_HOLDS_DOCUMENT(&array_iter)) {
                bson_iter_t doc_iter;
                bson_iter_recurse(&array_iter, &doc_iter);
                if (bson_iter_find(&doc_iter, "_id") && BSON_ITER_HOLDS_UTF8(&doc_iter)) {
                    ids_to_remove.emplace_back(bson_iter_utf8(&doc_iter, nullptr));
                }
            }
        }
    }

    if (ids_to_remove.empty()) {
        aevum::util::log::Logger::info("Core: Remove query for collection '" + std::string(coll) +
                                       "' matched 0 documents. No action taken.");
        return {aevum::util::Status::NotFound("No documents matched the query."), 0};
    }
    aevum::util::log::Logger::debug("Core: Identified " + std::to_string(ids_to_remove.size()) +
                                    " documents to remove from collection '" + std::string(coll) +
                                    "'.");

    int removed_count = 0;
    for (const auto &uuid : ids_to_remove) {
        auto target_doc_opt = index_manager_.get_document_by_id(coll, uuid);
        if (target_doc_opt) {
            if (auto status = storage_.remove(coll, uuid); !status.ok()) {
                aevum::util::log::Logger::warn("Core: Failed to remove document '" + uuid +
                                               "' from storage. Status: " + status.to_string());
                continue;
            }
            index_manager_.remove_document_from_indexes(coll, *target_doc_opt);
            removed_count++;
        }
    }

    aevum::util::log::Logger::info("Core: Successfully removed " + std::to_string(removed_count) +
                                   " documents from collection '" + std::string(coll) + "'.");
    return {aevum::util::Status::OK(), removed_count};
}

/**
 * @brief Sets or updates the schema for a collection.
 * @param coll The target collection name.
 * @param schema A BSON document defining the schema.
 * @return `Status::OK()` on success.
 */
aevum::util::Status Core::set_schema(std::string_view coll,
                                     const aevum::bson::doc::Document &schema) {
    aevum::util::log::Logger::info("Core: Setting/updating schema for collection '" +
                                   std::string(coll) + "'.");
    return schema_manager_.set_schema(coll, schema);
}

/**
 * @brief Creates a new secondary index on a field within a collection.
 * @param coll The target collection name.
 * @param field The field to create an index on.
 * @return `Status::OK()` on success.
 */
aevum::util::Status Core::create_index(std::string_view coll, std::string_view field) {
    aevum::util::log::Logger::info("Core: Creating index on field '" + std::string(field) +
                                   "' for collection '" + std::string(coll) + "'.");
    std::vector<aevum::bson::doc::Document> existing_docs = storage_.load_collection(coll);
    return index_manager_.create_index(coll, field, existing_docs);
}

/**
 * @brief Creates a new user and persists their credentials.
 * @param raw_key The plain-text API key for the new user.
 * @param role The assigned role for the new user.
 * @return `Status::OK()` on success.
 */
aevum::util::Status Core::create_user(std::string_view raw_key, auth::UserRole role) {
    std::string role_str(auth::to_string(role));
    aevum::util::log::Logger::info("Core: Creating new user with role '" + role_str + "'.");
    std::string key_hash = aevum::util::hash::djb2_string(raw_key);
    auth_manager_.add_user(key_hash, role);

    bson_t *b = bson_new();
    BSON_APPEND_UTF8(b, "_id", aevum::util::uuid::generate_v4().c_str());
    BSON_APPEND_UTF8(b, "key_hash", key_hash.c_str());
    BSON_APPEND_UTF8(b, "role", role_str.c_str());

    return storage_.put("_auth", key_hash, aevum::bson::doc::Document(b));
}

/**
 * @brief Authenticates a user's raw API key.
 * @param raw_key The plain-text API key.
 * @return The `UserRole` if authentication is successful, `UserRole::NONE` otherwise.
 */
auth::UserRole Core::authenticate(std::string_view raw_key) const {
    return auth_manager_.authenticate(raw_key);
}

}  // namespace aevum::db
