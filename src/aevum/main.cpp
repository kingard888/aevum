// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file main.cpp
 * @brief The primary orchestration entry point for the AevumDB daemon process.
 * @details This file implements the high-level lifecycle management of the AevumDB server. It
 * is responsible for the systematic initialization of the core database engine, the activation
 * of the multi-threaded network server, and the configuration of a robust POSIX signal-handling
 * mechanism. The architecture employed here ensures that the database maintains structural
 * integrity by intercepting termination signals and initiating a coordinated, graceful shutdown
 * sequence across all active subsystems.
 */
#include <csignal>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <thread>

#include "aevum/client/net/server.hpp"
#include "aevum/db/core/core.hpp"
#include "aevum/util/log/logger.hpp"

/**
 * @namespace aevum::daemon
 * @brief Logic and state management specific to the execution of the AevumDB background daemon.
 */
namespace aevum::daemon {

/**
 * @brief A global pointer to the active server instance, utilized for signal-driven management.
 * @details This static pointer facilitates the communication between the asynchronous signal
 * handling thread and the main network server instance. It allows the `signal_wait_thread` to
 * invoke the `stop()` method on the server, ensuring that the network listener and all worker
 * threads are terminated correctly upon receipt of external interruption signals.
 */
static aevum::net::server::Server *g_server_instance = nullptr;

/**
 * @brief The execution routine for the dedicated system signal monitoring thread.
 * @details This thread function employs a synchronous signal handling model using `sigwait`.
 * It masks the `SIGINT` and `SIGTERM` signals in the main execution path and delegates their
 * management to this specialized thread. Upon receiving a signal, it logs the event with high
 * technical detail and triggers the graceful shutdown of the server instance pointed to by
 * `g_server_instance`. This approach avoids the inherent risks of asynchronous signal
 * handlers (reentrancy issues) and ensures thread-safe application termination.
 */
void signal_wait_thread() {
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGINT);   // Intercept the interrupt signal (typically Ctrl+C).
    sigaddset(&signal_set, SIGTERM);  // Intercept the software termination signal.

    aevum::util::log::Logger::info(
        "System: Synchronous signal monitoring thread has been initialized and is now active.");

    int signal_number;
    // Block until one of the signals in the set is delivered to the process.
    if (sigwait(&signal_set, &signal_number) == 0) {
        aevum::util::log::Logger::warn("System: Interruption event detected. Received signal " +
                                       std::to_string(signal_number) +
                                       ". Initiating the global graceful shutdown sequence...");
        if (g_server_instance) {
            g_server_instance->stop();
        }
    }
}

/**
 * @brief Emits the standardized command-line interface usage guide to the standard output stream.
 * @details This utility function provides clarity on the available configuration parameters for
 * the daemon, specifically the physical data storage path and the network port allocation.
 * @param binary_name The filename of the current executable as provided by the operating system.
 */
void print_help(const char *binary_name) {
    std::cout << "AevumDB Daemon - Command Line Interface Guide\n"
              << "Usage: " << binary_name << " [DATA_PATH] [PORT]\n"
              << "  DATA_PATH : (Optional) The filesystem directory allocated for database "
                 "persistence. Defaults to './aevum_data'.\n"
              << "  PORT      : (Optional) The TCP/IP port number for the network listener. "
                 "Defaults to 55001.\n";
}

}  // namespace aevum::daemon

/**
 * @brief The canonical entry point of the AevumDB daemon application.
 * @details The execution sequence begins by masking terminal signals (`SIGINT`, `SIGTERM`)
 * to prevent default asynchronous interruption of the process. It then transitions into the
 * configuration phase, where it parses command-line arguments to define the operational
 * environment (storage directory and listening port).
 *
 * The core initialization phase instantiates the `aevum::db::Core` engine, which prepares
 * the storage and indexing layers. Subsequently, the `aevum::net::server::Server` is
 * initialized and linked to the database core. A dedicated signal-handling thread is then
 * spawned to monitor system-level termination requests.
 *
 * Finally, the application enters the blocking server loop via `network_server.run()`. This
 * loop persists until an external signal is received or a critical failure occurs, at which
 * point the application performs an exhaustive cleanup, joins all threads, and exits with a
 * status code reflecting the success or failure of the operation.
 *
 * @param argc The count of command-line arguments.
 * @param argv The vector of command-line argument strings.
 * @return Returns `0` if the daemon completes its lifecycle and shuts down without error.
 * @return Returns `1` if a critical exception or fatal initialization error is encountered.
 */
int main(int argc, char *argv[]) {
    // Configure the initial signal mask for the main thread and all its future descendants.
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signal_set, nullptr);

    // Evaluate if the user requested the manual.
    if (argc > 1 && std::string(argv[1]) == "--help") {
        aevum::daemon::print_help(argv[0]);
        return 0;
    }

    // Initialize global logging parameters.
    aevum::util::log::Logger::set_level(aevum::util::log::LogLevel::INFO);
    std::string data_path = "./aevum_data";
    int port = 55001;

    try {
        // Dynamically resolve configuration from command-line arguments.
        if (argc > 1) data_path = argv[1];
        if (argc > 2) port = std::stoi(argv[2]);

        aevum::util::log::Logger::info("Daemon: Commencing AevumDB daemon bootstrap sequence...");

        // Initialize the central database orchestration engine.
        aevum::db::Core database_instance(data_path);

        // Configure the high-performance network server subsystem.
        aevum::net::server::Server network_server(database_instance, port);

        // Register the server instance with the global signal manager.
        aevum::daemon::g_server_instance = &network_server;

        // Activate the dedicated signal monitoring thread.
        std::thread signal_thread(aevum::daemon::signal_wait_thread);

        try {
            // Enter the main blocking loop of the network server.
            network_server.run();
        } catch (...) {
            // In the event of an unhandled exception within the server loop,
            // ensure the signal thread is detached to prevent application hang.
            if (signal_thread.joinable()) {
                signal_thread.detach();
            }
            throw;  // Re-propagate the exception for global error handling.
        }

        // Wait for the signal handling thread to conclude its shutdown duties.
        if (signal_thread.joinable()) {
            signal_thread.join();
        }

    } catch (const std::exception &e) {
        aevum::util::log::Logger::fatal("Daemon: A critical runtime exception has occurred: " +
                                        std::string(e.what()));
        return 1;
    }

    aevum::util::log::Logger::info(
        "Daemon: AevumDB daemon has successfully concluded all operations. Exiting gracefully.");
    return 0;
}
