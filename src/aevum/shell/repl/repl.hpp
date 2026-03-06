// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file repl.hpp
 * @brief Declares the core functions for managing the AevumDB interactive shell's
 * Read-Eval-Print Loop (REPL).
 * @details This header defines the public interface for the REPL component, which is responsible
 * for the user-facing interactive session. It provides a clean separation between the main
 * application entry point and the continuous loop of user interaction.
 */
#pragma once

#include <string>

// Forward-declare AevumClient to avoid including the full header, which helps
// reduce compilation times and prevent potential circular dependencies.
namespace aevum::client {
class AevumClient;
}

namespace aevum::shell::repl {

/**
 * @brief Displays a comprehensive, well-formatted help message to the console.
 * @details This function outputs a detailed guide for the user, listing all available shell
 * commands, their syntax, expected arguments, and examples. It serves as the primary
 * on-demand documentation for the interactive shell.
 */
void print_help();

/**
 * @brief Initiates and manages the main Read-Eval-Print Loop (REPL) for the interactive shell.
 * @details This function is the heart of the interactive client. It enters a continuous loop that:
 * 1.  **Reads**: Prompts the user with `> ` and reads a full line of input.
 * 2.  **Evaluates**: Trims the input, checks for built-in commands (`exit`, `quit`, `help`), and if
 *     it's a database command, dispatches it to the `command_parser` for processing.
 * 3.  **Prints**: The results or errors from the command processing are printed to the console.
 * The loop continues until the user signals an exit (e.g., by typing 'exit' or via Ctrl+D).
 * @param client A reference to an active and connected `AevumClient` instance, which will be
 *        used to send all database commands to the server.
 */
void run(client::AevumClient &client);

}  // namespace aevum::shell::repl
