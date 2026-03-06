# Copyright (c) 2026 Ananda Firmansyah.
# Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

# Define the minimum required version of CMake to ensure compatibility with modern features.
cmake_minimum_required(VERSION 3.21)

# Initialize the AevumDB project, specifying support for C++, C, and Assembly.
# Assembly support is required for certain low-level optimizations in third-party libraries.
project(aevumdb CXX C ASM)

# Configure the fundamental C++ language standards and compilation requirements.
# We utilize C++17 to leverage modern features like std::string_view, std::shared_mutex,
# and structured bindings, which are critical for the performance of the engine.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define the centralized output locations for the resulting binaries and libraries.
# This ensures a clean build artifact structure within the binary directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Integration of Third-Party Dependencies
# This section orchestrates the inclusion of external libraries required for BSON
# serialization, high-performance JSON parsing, and physical data persistence.

include(ExternalProject)

# Integrate 'simdjson' for exceptionally fast, SIMD-accelerated JSON parsing.
# It is used primarily by the network server and the interactive shell.
message(STATUS "Integrating the simdjson library for high-performance parsing...")
add_subdirectory(third_party/simdjson EXCLUDE_FROM_ALL)

# Integrate the 'mongo-c-driver' solely for its 'libbson' component.
# AevumDB utilizes BSON as its primary binary storage and transmission format.
message(STATUS "Integrating the MongoDB C Driver (BSON component)...")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(ENABLE_MONGOC OFF CACHE BOOL "Build mongoc" FORCE)
set(ENABLE_BSON ON CACHE BOOL "Build bson" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "Build static" FORCE)
add_subdirectory(third_party/mongo-c-driver EXCLUDE_FROM_ALL)

# Integrate WiredTiger as an ExternalProject to isolate its complex build environment.
message(STATUS "Configuring WiredTiger as an ExternalProject...")
set(WIREDTIGER_PREFIX "${CMAKE_BINARY_DIR}/third_party/wiredtiger")
set(WIREDTIGER_INSTALL_DIR "${WIREDTIGER_PREFIX}/install")
set(WIREDTIGER_INCLUDE_DIR "${WIREDTIGER_INSTALL_DIR}/include")
set(WIREDTIGER_STATIC_LIB "${WIREDTIGER_INSTALL_DIR}/lib/libwiredtiger.a")

ExternalProject_Add(wiredtiger_project
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/wiredtiger"
    PREFIX "${WIREDTIGER_PREFIX}"
    INSTALL_DIR "${WIREDTIGER_INSTALL_DIR}"
    PATCH_COMMAND sed -i "/^# /! s/add_subdirectory(test\\/fail_fs)/# add_subdirectory(test\\/fail_fs)/g" ext/CMakeLists.txt && sed -i "/^# /! s/add_subdirectory(test\\/key_provider)/# add_subdirectory(test\\/key_provider)/g" ext/CMakeLists.txt
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${WIREDTIGER_INSTALL_DIR}
        -DENABLE_SHARED:BOOL=OFF
        -DENABLE_STATIC:BOOL=ON
        -DENABLE_PYTHON:BOOL=OFF
        -DHAVE_UNITTEST:BOOL=OFF
        -DHAVE_UNITTEST_ASSERTS:BOOL=OFF
        -DENABLE_CPPSUITE:BOOL=OFF
        -DENABLE_MODEL:BOOL=OFF
        -DENABLE_LAZYFS:BOOL=OFF
        -DENABLE_TESTING:BOOL=OFF
        -DENABLE_EXAMPLES:BOOL=OFF
        -DENABLE_EXTENSIONS:BOOL=OFF
        -DENABLE_STRICT:BOOL=OFF
        -DWITH_PIC:BOOL=ON
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    BUILD_BYPRODUCTS "${WIREDTIGER_STATIC_LIB}"
)

# WiredTiger is now integrated as a target 'wt::wiredtiger'
set(HAVE_WIREDTIGER TRUE)
add_definitions(-DHAVE_WIREDTIGER)

# Integration of the Rust-based Query Execution Engine (FFI)
# This section automates the compilation of the Rust FFI library using Cargo.

find_program(CARGO_EXECUTABLE cargo)
if(CARGO_EXECUTABLE)
    message(STATUS "Found Cargo: ${CARGO_EXECUTABLE}. Configuring Rust FFI build...")
    
    set(RUST_FFI_DIR "${CMAKE_SOURCE_DIR}/src/aevum/ffi")
    set(RUST_FFI_LIB "${RUST_FFI_DIR}/target/release/libaevum_ffi.a")
    
    add_custom_command(
        OUTPUT "${RUST_FFI_LIB}"
        COMMAND ${CARGO_EXECUTABLE} build --release
        WORKING_DIRECTORY "${RUST_FFI_DIR}"
        COMMENT "Building Rust FFI library in release mode..."
        VERBATIM
    )
    
    add_custom_target(rust_ffi_target DEPENDS "${RUST_FFI_LIB}")
else()
    message(WARNING "Cargo not found. Rust FFI components will not be built automatically.")
endif()

# Source Discovery and Classification
# This section programmatically identifies all relevant source files and categorizes them
# into common engine components and specialized application entry points.

# Establish the root inclusion path for the AevumDB source tree.
include_directories(${CMAKE_SOURCE_DIR}/src)

# Recursively identify all C++ implementation files within the project's source tree.
# This includes the recently modularized core/ and shell/ sub-components.
file(GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/aevum/**/*.cpp"
)

# Filter out 'main.cpp' files from the general source list.
# These files contain distinct entry points for the daemon and the shell, and
# must be handled individually to avoid multiple-definition linker errors.
list(FILTER ALL_SOURCES EXCLUDE REGEX "main\\.cpp$")

# Define the explicit entry points for the daemon and the interactive shell.
set(DAEMON_MAIN "${CMAKE_SOURCE_DIR}/src/aevum/main.cpp")
set(SHELL_MAIN "${CMAKE_SOURCE_DIR}/src/aevum/shell/main.cpp")

# Compilation and Linking for the AevumDB Daemon
# The 'aevumdb' target represents the central database server process.

message(STATUS "Configuring build for the AevumDB Daemon (aevumdb)...")
add_executable(aevumdb ${DAEMON_MAIN} ${ALL_SOURCES})
add_dependencies(aevumdb wiredtiger_project)
if(CARGO_EXECUTABLE)
    add_dependencies(aevumdb rust_ffi_target)
endif()

# Define target-specific inclusion paths for headers.
target_include_directories(aevumdb PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/simdjson/include
    ${CMAKE_SOURCE_DIR}/third_party/mongo-c-driver/src/libbson/src/bson
    ${CMAKE_BINARY_DIR}/third_party/mongo-c-driver/src/libbson/src/bson
    ${WIREDTIGER_INCLUDE_DIR}
)

# Associate the daemon with its required static and system libraries.
target_link_libraries(aevumdb PRIVATE
    simdjson
    bson_static
    "${WIREDTIGER_STATIC_LIB}"
    pthread
    m
    dl
    rt
)

# Link the Rust FFI library if it was successfully configured.
if(CARGO_EXECUTABLE)
    target_link_libraries(aevumdb PRIVATE "${RUST_FFI_LIB}")
else()
    # Check for pre-existing library if Cargo wasn't found.
    set(PREBUILT_RUST_FFI "${CMAKE_SOURCE_DIR}/src/aevum/ffi/target/release/libaevum_ffi.a")
    if(EXISTS "${PREBUILT_RUST_FFI}")
        target_link_libraries(aevumdb PRIVATE "${PREBUILT_RUST_FFI}")
    endif()
endif()

# Apply platform-specific compilation flags to ensure optimal code generation.
if(NOT MSVC)
    target_compile_options(aevumdb PRIVATE -Wall -Wextra -O3)
else()
    target_compile_options(aevumdb PRIVATE /W4)
endif()

# Compilation and Linking for the AevumDB Interactive Shell
# The 'aevumsh' target provides the interactive user interface to the database.

message(STATUS "Configuring build for the AevumDB Shell (aevumsh)...")
add_executable(aevumsh ${SHELL_MAIN} ${ALL_SOURCES})
add_dependencies(aevumsh wiredtiger_project)
if(CARGO_EXECUTABLE)
    add_dependencies(aevumsh rust_ffi_target)
endif()

# Define target-specific inclusion paths for headers.
target_include_directories(aevumsh PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/simdjson/include
    ${CMAKE_SOURCE_DIR}/third_party/mongo-c-driver/src/libbson/src/bson
    ${CMAKE_BINARY_DIR}/third_party/mongo-c-driver/src/libbson/src/bson
    ${WIREDTIGER_INCLUDE_DIR}
)

# Associate the shell with its required libraries, maintaining consistency with the daemon.
target_link_libraries(aevumsh PRIVATE
    simdjson
    bson_static
    "${WIREDTIGER_STATIC_LIB}"
    pthread
    m
    dl
    rt
)

# Link the Rust FFI library if it was successfully configured.
if(CARGO_EXECUTABLE)
    target_link_libraries(aevumsh PRIVATE "${RUST_FFI_LIB}")
else()
    if(EXISTS "${PREBUILT_RUST_FFI}")
        target_link_libraries(aevumsh PRIVATE "${PREBUILT_RUST_FFI}")
    endif()
endif()

# Apply optimization and warning flags for the shell executable.
if(NOT MSVC)
    target_compile_options(aevumsh PRIVATE -Wall -Wextra -O3)
else()
    target_compile_options(aevumsh PRIVATE /W4)
endif()

# Final Deployment and Configuration Summary

# Define the installation destination for the produced binaries.
install(TARGETS aevumdb aevumsh DESTINATION bin)

# Output a comprehensive summary of the current build configuration for verification.
message(STATUS "AevumDB Build Configuration Summary")
message(STATUS "Project Root:    ${CMAKE_SOURCE_DIR}")
message(STATUS "Output Path:      ${CMAKE_BINARY_DIR}")
message(STATUS "Daemon Artifact: ${CMAKE_BINARY_DIR}/bin/aevumdb")
message(STATUS "Shell Artifact:  ${CMAKE_BINARY_DIR}/bin/aevumsh")
message(STATUS "C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "Configuration complete.")
