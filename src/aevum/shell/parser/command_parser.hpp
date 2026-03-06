// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file command_parser.hpp
 * @brief Declares the core command processing function for the AevumDB interactive shell.
 * @details This header defines the interface for the `parser` component, which is responsible
 * for dissecting raw user command strings into executable database operations. It serves as the
 * bridge between the user-facing REPL and the programmatic `AevumClient` API.
 */
#pragma once

#include <string>

#include "aevum/client/aevum_client.hpp"

/**
 * @namespace aevum::shell::parser
 * @brief Encapsulates all functionality related to parsing and interpreting commands from the
 * interactive shell.
 */
namespace aevum::shell::parser {

/**
 * @brief Parses a complete command line string, translates it into a database operation,
 * executes it via the client, and formats the response for display.
 * @details This is the primary logical engine of the shell. It deconstructs a command string
 * (e.g., "db.users.find({ 'age': 25 })") into its constituent parts: the collection ("users"),
 * the operation ("find"), and the arguments ("{ 'age': 25 }").
 *
 * It contains specialized logic to handle the unique syntax of different commands, including
 * robustly parsing nested JSON objects. Once parsed, it invokes the corresponding method on the
 * `AevumClient` instance (e.g., `client.find(...)`). Finally, it receives the raw JSON response
 * from the client, parses it, and formats it into a human-readable summary for printing to
 * the console.
 *
 * @param line The complete, trimmed command line string provided by the user.
 * @param client A reference to the active `AevumClient` instance, which will be used to
 *        execute the command against the database server.
 */
void process_command(const std::string &line, client::AevumClient &client);

}  // namespace aevum::shell::parser
