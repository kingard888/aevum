// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file aevum_client.cpp
 * @brief Implements the high-level `AevumClient` class for interacting with the database server.
 * @details This file provides the concrete implementations for the `AevumClient` methods. It
 * handles the orchestration of building request payloads and dispatching them through the
 * underlying network connection, abstracting these details from the end-user.
 */
#include "aevum/client/aevum_client.hpp"

namespace aevum::client {

/**
 * @brief Constructs an `AevumClient` instance, initializing connection parameters and credentials.
 * @details This constructor initializes the underlying `net::client::Connection` object with the
 * server's host and port, and securely stores the API key for authenticating all subsequent
 * requests.
 * @param host The hostname or IP address of the AevumDB server.
 * @param port The port number of the AevumDB server.
 * @param api_key The API key for authenticating with the server.
 */
AevumClient::AevumClient(std::string host, int port, std::string api_key)
    : conn_(std::move(host), port), api_key_(std::move(api_key)) {}

/**
 * @brief Establishes a persistent connection to the AevumDB server.
 * @return `true` if the connection is successfully established or already active, `false`
 * otherwise.
 */
bool AevumClient::connect() { return conn_.connect_server(); }

/**
 * @brief Disconnects from the AevumDB server in a graceful manner.
 * @details To ensure a clean session termination, this method first sends a formal "exit" command
 * to the server before instructing the underlying connection object to close the socket.
 */
void AevumClient::disconnect() {
    if (conn_.is_connected()) {
        (void)conn_.send_request(build_payload("exit", "", ""));
        conn_.disconnect_server();
    }
}

/**
 * @brief A private helper to construct the standardized JSON payload for any request.
 * @details This function is central to client communication. It programmatically builds a JSON
 * string, ensuring all requests adhere to the server's expected format. It always includes the
 * `auth` and `action` fields. The `collection` field and any `extra_fields` are conditionally
 * included only if they are not empty, resulting in a clean and efficient payload.
 * @param action The specific database action being requested (e.g., "insert", "find").
 * @param collection The target collection for the action.
 * @param extra_fields A pre-formatted string containing additional JSON key-value pairs specific
 *        to the action (e.g., `"query":{...}, "sort":{...}`).
 * @return A complete, valid JSON payload as a `std::string`.
 */
std::string AevumClient::build_payload(std::string_view action, std::string_view collection,
                                       std::string_view extra_fields) const {
    std::string payload = "{";
    payload.reserve(128 + api_key_.length() + action.length() + collection.length() +
                    extra_fields.length());

    payload += "\"auth\":\"" + api_key_ + "\",";
    payload += "\"action\":\"" + std::string(action) + "\"";

    if (!collection.empty()) {
        payload += ",\"collection\":\"" + std::string(collection) + "\"";
    }

    if (!extra_fields.empty()) {
        payload += ",";
        payload += extra_fields;
    }

    payload += "}";
    return payload;
}

/**
 * @brief Packages and sends a document insertion request to the server.
 * @param collection The name of the collection into which the document will be inserted.
 * @param doc_json The JSON string of the document to insert.
 * @return The server's response, typically indicating success and the new document's `_id`.
 */
std::string AevumClient::insert(std::string_view collection, std::string_view doc_json) {
    std::string extra = "\"data\":" + std::string(doc_json);
    return conn_.send_request(build_payload("insert", collection, extra));
}

/**
 * @brief Packages and sends a document find request with full query options.
 * @param collection The collection to query.
 * @param query_json The query filter criteria.
 * @param sort_json The sort order specification.
 * @param limit The maximum number of results to return.
 * @param skip The number of results to skip.
 * @return The server's response, typically a JSON array of matching documents.
 */
std::string AevumClient::find(std::string_view collection, std::string_view query_json,
                              std::string_view sort_json, int64_t limit, int64_t skip) {
    std::string extra = R"("query":)" + std::string(query_json) + ",";
    extra += R"("sort":)" + std::string(sort_json) + ",";
    extra += R"("limit":)" + std::to_string(limit) + ",";
    extra += R"("skip":)" + std::to_string(skip);

    return conn_.send_request(build_payload("find", collection, extra));
}

/**
 * @brief Packages and sends a document update request.
 * @param collection The collection where the update will occur.
 * @param query_json The filter to select which documents to update.
 * @param update_json The update operations to apply.
 * @return The server's response, typically indicating the number of documents modified.
 */
std::string AevumClient::update(std::string_view collection, std::string_view query_json,
                                std::string_view update_json) {
    std::string extra = R"("query":)" + std::string(query_json) + ",";
    extra += R"("update":)" + std::string(update_json);
    return conn_.send_request(build_payload("update", collection, extra));
}

/**
 * @brief Packages and sends a document removal request.
 * @param collection The collection from which to remove documents.
 * @param query_json The filter to select which documents to remove.
 * @return The server's response, typically indicating the number of documents removed.
 */
std::string AevumClient::remove(std::string_view collection, std::string_view query_json) {
    std::string extra = R"("query":)" + std::string(query_json);
    return conn_.send_request(build_payload("delete", collection, extra));
}

/**
 * @brief Packages and sends a document count request.
 * @param collection The collection in which to count documents.
 * @param query_json The filter to select which documents to count.
 * @return The server's response, containing the total count.
 */
std::string AevumClient::count(std::string_view collection, std::string_view query_json) {
    std::string extra = R"("query":)" + std::string(query_json);
    return conn_.send_request(build_payload("count", collection, extra));
}

}  // namespace aevum::client
