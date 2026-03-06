// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file aevum_client.hpp
 * @brief Declares the `AevumClient` class, a high-level, user-facing API for interacting with an
 * AevumDB server.
 * @details This header defines the primary entry point for client applications. The `AevumClient`
 * class provides a clean, modern C++ interface that abstracts away all the underlying networking
 * complexities and request/response formatting, offering a set of intuitive methods that correspond
 * directly to database operations.
 */
#pragma once

#include <string>
#include <string_view>

#include "aevum/client/net/connection.hpp"

namespace aevum::client {

/**
 * @class AevumClient
 * @brief Provides a simplified, high-level API for all client-side interactions with an AevumDB
 * server.
 *
 * @details This class serves as the main facade for client applications. It encapsulates an
 * instance of the low-level `net::client::Connection` and provides a suite of methods
 * (`insert`, `find`, `update`, etc.) that mirror the database's core functionalities. It handles
 * the automatic construction of JSON request payloads, including the injection of the user's API
 * key for authentication, and forwards them to the connection manager for transmission.
 */
class AevumClient {
  public:
    /**
     * @brief Constructs an `AevumClient` instance, configured to communicate with a specific
     * server.
     * @param host The hostname or IPv4 address of the AevumDB server.
     * @param port The TCP port number on which the server is listening.
     * @param api_key The API key that will be used to authenticate all requests sent from this
     * client.
     */
    AevumClient(std::string host, int port, std::string api_key);

    /**
     * @brief Default destructor.
     */
    ~AevumClient() = default;

    /**
     * @brief Establishes the underlying TCP connection to the AevumDB server.
     * @return `true` if the connection is successful or was already established; `false` otherwise.
     */
    [[nodiscard]] bool connect();

    /**
     * @brief Gracefully disconnects from the AevumDB server.
     * @details This method first sends an "exit" command to the server to signal a clean shutdown
     * of the session before closing the underlying network socket.
     */
    void disconnect();

    /**
     * @brief Sends a request to insert a new document into a collection.
     * @param collection The name of the target collection.
     * @param doc_json A `std::string_view` containing the JSON representation of the document to
     * insert.
     * @return A `std::string` containing the raw JSON response from the server.
     */
    [[nodiscard]] std::string insert(std::string_view collection, std::string_view doc_json);

    /**
     * @brief Sends a request to find documents in a collection based on a complex query.
     * @param collection The name of the target collection.
     * @param query_json The JSON string defining the filter criteria.
     * @param sort_json The JSON string defining the sort order. Defaults to an empty object for no
     * sort.
     * @param limit The maximum number of documents to return. Defaults to 0 (no limit).
     * @param skip The number of documents to skip. Defaults to 0.
     * @return A `std::string` containing the server's raw JSON response, typically an array of
     * documents.
     */
    [[nodiscard]] std::string find(std::string_view collection, std::string_view query_json,
                                   std::string_view sort_json = "{}", int64_t limit = 0,
                                   int64_t skip = 0);

    /**
     * @brief Sends a request to update documents in a collection that match a given query.
     * @param collection The name of the target collection.
     * @param query_json The JSON string defining the filter for documents to be updated.
     * @param update_json The JSON string specifying the update operations to apply.
     * @return A `std::string` containing the server's raw JSON response.
     */
    [[nodiscard]] std::string update(std::string_view collection, std::string_view query_json,
                                     std::string_view update_json);

    /**
     * @brief Sends a request to remove documents from a collection that match a given query.
     * @param collection The name of the target collection.
     * @param query_json The JSON string defining the filter for documents to be removed.
     * @return A `std::string` containing the server's raw JSON response.
     */
    [[nodiscard]] std::string remove(std::string_view collection, std::string_view query_json);

    /**
     * @brief Sends a request to count the number of documents in a collection that match a query.
     * @param collection The name of the target collection.
     * @param query_json The JSON string defining the filter criteria for the count.
     * @return A `std::string` containing the server's raw JSON response, typically an object with a
     * "count" field.
     */
    [[nodiscard]] std::string count(std::string_view collection, std::string_view query_json);

  private:
    /// The underlying network connection manager responsible for all TCP communication.
    net::client::Connection conn_;
    /// The API key used for authenticating all outgoing requests.
    std::string api_key_;

  public:  // Public for shell/CLI usage
    /**
     * @brief A utility function to construct a standardized JSON request payload.
     * @details This method assembles the final JSON string sent to the server. It combines the
     * API key, the requested action, the target collection, and any action-specific fields into
     * the required JSON object format.
     * @param action The database action (e.g., "insert", "find").
     * @param collection The target collection name.
     * @param extra_fields A string containing any additional, pre-formatted JSON fields required
     *        by the specific action (e.g., `"data":{...}` or `"query":{...}`).
     * @return A complete JSON request payload as a `std::string`.
     */
    [[nodiscard]] std::string build_payload(std::string_view action, std::string_view collection,
                                            std::string_view extra_fields) const;

    /**
     * @brief A low-level method to send a pre-constructed request payload to the server.
     * @details This bypasses the client's payload-building logic and sends the provided string
     * directly to the server, returning the raw response.
     * @param payload The complete JSON request payload string.
     * @return The raw JSON response string from the server.
     */
    [[nodiscard]] std::string send_request(std::string_view payload) {
        return conn_.send_request(payload);
    }
};

}  // namespace aevum::client
