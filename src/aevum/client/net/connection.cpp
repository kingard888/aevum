// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file connection.cpp
 * @brief Implements the low-level TCP client connection logic for AevumDB.
 * @details This file provides the concrete implementations for the `Connection` class,
 * encapsulating the POSIX socket API calls required to establish, maintain, and communicate
 * over a client-side TCP connection.
 */
#include "aevum/client/net/connection.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace aevum::net::client {

/**
 * @brief Constructs a `Connection` object, initializing the server endpoint details.
 * @param host The hostname or IPv4 address of the server to connect to. This value is moved.
 * @param port The target TCP port number on the server.
 */
Connection::Connection(std::string host, int port)
    : host_(std::move(host)), port_(port), socket_fd_(-1) {}

/**
 * @brief Destroys the `Connection` object, ensuring the socket is gracefully closed.
 * @details The destructor guarantees that `disconnect_server()` is called, which prevents
 * resource leaks by closing any open socket file descriptor.
 */
Connection::~Connection() { disconnect_server(); }

/**
 * @brief Establishes a TCP connection to the specified server.
 * @details This method is idempotent. If a valid socket connection already exists, it returns
 * `true` immediately. Otherwise, it proceeds with the full connection sequence:
 * 1. Creates a new TCP socket (`socket`).
 * 2. Configures the `sockaddr_in` struct with the server's address family, port, and IP address.
 *    It uses `inet_pton` to convert the string IP address to the required binary format.
 * 3. Attempts to connect to the server using `connect`.
 * If any step fails, the socket is cleaned up, and the function returns `false`.
 * @return `true` if the connection is successfully established or already exists; `false`
 * otherwise.
 */
bool Connection::connect_server() {
    if (is_connected()) return true;

    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        // In a real-world scenario, this would log a detailed error.
        return false;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        disconnect_server();
        return false;
    }

    if (connect(socket_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        disconnect_server();
        return false;
    }

    return true;
}

/**
 * @brief Closes the active socket connection.
 * @details This function checks if the `socket_fd_` is valid (>= 0) before attempting to `close()`
 * it. After closing, it resets `socket_fd_` to -1 to mark the connection as inactive. This makes
 * the operation safe to call multiple times.
 */
void Connection::disconnect_server() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

/**
 * @brief Checks the state of the connection.
 * @return `true` if the socket file descriptor is valid (not -1), indicating an active connection.
 */
bool Connection::is_connected() const noexcept { return socket_fd_ >= 0; }

/**
 * @brief Transmits a request to the server and retrieves the response.
 * @details This function first ensures a connection is active by calling `connect_server()`. If
 * the connection fails, it returns a descriptive JSON error. It then sends the entire `payload`
 * over the socket. Following the send, it blocks and waits to `recv` the server's response into a
 * fixed-size buffer. If any network operation fails, the connection is terminated, and an
 * appropriate JSON error is returned.
 * @param payload The data to be sent to the server.
 * @return A `std::string` containing the server's response or a JSON error object.
 */
std::string Connection::send_request(std::string_view payload) {
    if (!is_connected()) {
        if (!connect_server()) {
            return R"({"status":"error","message":"Connection Failure: Unable to establish connection to AevumDB server."})";
        }
    }

    ssize_t bytes_sent = send(socket_fd_, payload.data(), payload.length(), 0);
    if (bytes_sent < 0) {
        disconnect_server();
        return R"({"status":"error","message":"Network Error: Failed to send data to server."})";
    }

    char buffer[16384];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        disconnect_server();
        // A return value of 0 from recv indicates a graceful shutdown by the peer.
        if (bytes_read == 0) {
            return R"({"status":"error","message":"Connection Error: Connection was closed by the server."})";
        }
        return R"({"status":"error","message":"Network Error: Failed to receive response from server."})";
    }

    return std::string(buffer, bytes_read);
}

}  // namespace aevum::net::client
