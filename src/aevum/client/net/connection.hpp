// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file connection.hpp
 * @brief Declares the `Connection` class, a low-level abstraction for managing a client-side
 * TCP socket connection to an AevumDB server.
 * @details This header defines the core networking component for the AevumDB client. The
 * `Connection` class encapsulates the fundamental operations of POSIX sockets, including
 * connection, disconnection, and data transmission, providing a simplified and robust
- * interface for higher-level client logic.
 */
#pragma once

#include <string>
#include <string_view>

namespace aevum::net::client {

/**
 * @class Connection
 * @brief Manages the complete lifecycle of a single, stateful TCP socket connection to a remote
 * AevumDB server.
 *
 * @details This class provides a foundational abstraction over the underlying C-style socket API.
 * It is responsible for creating a socket, resolving the server's address, establishing a
 * connection, sending request payloads, and receiving response data. The class is designed to be
 * non-copyable to ensure that ownership of the socket file descriptor is unique and
 * unambiguous, preventing resource management conflicts. Its destructor guarantees that the
 * connection is gracefully closed, preventing resource leaks.
 */
class Connection {
  public:
    /**
     * @brief Constructs a `Connection` object configured for a specific server endpoint.
     * @param host The hostname or IPv4 address of the target AevumDB server.
     * @param port The TCP port number on which the server is listening.
     */
    Connection(std::string host, int port);

    /**
     * @brief Destroys the `Connection` object.
     * @details This destructor is critical for resource cleanup. It ensures that if a socket
     * connection is still active (`socket_fd_` is valid), `disconnect_server()` is called to
     * properly close the file descriptor and release system resources.
     */
    ~Connection();

    // Connection objects are non-copyable and non-movable to enforce exclusive ownership
    // of the underlying socket file descriptor and prevent resource management errors.
    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;
    Connection(Connection &&) = delete;
    Connection &operator=(Connection &&) = delete;

    /**
     * @brief Establishes a TCP connection to the configured AevumDB server.
     * @details This is an idempotent operation. If the connection is already active, it returns
     * `true` immediately. Otherwise, it performs the full sequence of socket creation, server
     * address resolution (`inet_pton`), and connection (`connect`).
     * @return `true` if the connection is successfully established or was already active.
     * @return `false` if any step in the connection process fails (e.g., socket creation,
     *         address resolution, or the `connect` call itself).
     */
    [[nodiscard]] bool connect_server();

    /**
     * @brief Disconnects from the server and closes the underlying socket file descriptor.
     * @details This method safely closes the active connection. It checks if the socket file
     * descriptor is valid before calling `close()` and then resets it to -1 to reflect the
     * disconnected state. It can be called multiple times without adverse effects.
     */
    void disconnect_server();

    /**
     * @brief Performs a non-blocking check to determine if the client is currently connected.
     * @return `true` if the socket file descriptor is valid (i.e., not -1), which implies an
     *         active or previously active connection. `false` otherwise.
     */
    [[nodiscard]] bool is_connected() const noexcept;

    /**
     * @brief Sends a request payload to the server and blocks until a response is received.
     * @details This is the primary method for client-server communication. If not currently
     * connected, it will first attempt to establish a connection via `connect_server()`. It then
     * sends the provided payload and waits to receive a response from the server into a fixed-size
     * buffer. In case of any send or receive error, it automatically disconnects.
     * @param payload A `std::string_view` representing the complete request data to be sent.
     * @return A `std::string` containing the raw response from the server. If a connection, send,
     *         or receive error occurs, a JSON-formatted error message is returned.
     */
    [[nodiscard]] std::string send_request(std::string_view payload);

  private:
    /// The hostname or IP address of the remote AevumDB server.
    std::string host_;
    /// The TCP port number of the remote AevumDB server.
    int port_;
    /// The underlying file descriptor for the client socket. A value of -1 indicates a disconnected
    /// state.
    int socket_fd_;
};

}  // namespace aevum::net::client
