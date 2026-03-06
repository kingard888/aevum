// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # AevumDB Query Engine Core
//!
//! This module, along with its sub-modules, constitutes the heart of the AevumDB query processing
//! engine. It provides a comprehensive suite of functionalities for the manipulation and retrieval
//! of data, all operating on in-memory `serde_json::Value` representations of documents. The engine
//! is architecturally designed for modularity and clarity, with each sub-module handling a distinct
//! and well-defined aspect of the query lifecycle.
//!
//! ## Architectural Overview
//!
//! The query engine is partitioned into the following key components:
//!
//! - **`operations`**: This is the highest-level module, exposing coarse-grained, user-facing
//!   operations that correspond to standard database interactions like `find`, `count`, `update`,
//!   and `delete`. It orchestrates the functionalities of the other modules to fulfill these requests.
//!
//! - **`matcher`**: Contains the core document-matching logic. It is responsible for recursively
//!   evaluating a query document against a data document to determine if it meets the specified criteria.
//!
//! - **`operators`**: Implements the evaluation logic for a rich set of MongoDB-style query operators
//!   (e.g., comparison operators like `$eq`, `$gt`, `$lt`, and type-checking operators like `$type`).
//!   This module gives the query language its expressive power.
//!
//! - **`comparator`**: Defines a deterministic and consistent comparison logic for all possible
//!   `serde_json::Value` types. This is essential for implementing correct and stable sorting
//!   behavior across heterogeneous data.
//!
//! - **`projection`**: Provides the functionality for shaping the output documents by selecting,
//!   including, or excluding specific fields from the result set.
//!
//! This modular design ensures that each component can be reasoned about, tested, and maintained
//! independently, while also allowing them to be composed into a powerful and flexible query processor.

/// Defines a consistent, total ordering for comparing different BSON/JSON value types,
/// which is fundamental for correct sorting behavior.
pub mod comparator;

/// Implements the core filtering logic, determining if a given document satisfies the
/// conditions specified in a query object.
pub mod matcher;

/// Orchestrates high-level, user-facing CRUD (Create, Read, Update, Delete) and validation
/// operations by composing the functionalities of the other query modules.
pub mod operations;

/// Contains the implementation for evaluating a rich set of query operators
/// (e.g., `$gt`, `$eq`, `$type`) against document fields.
pub mod operators;

/// Provides the logic for document projection, allowing for the selective inclusion or
/// exclusion of fields in query results.
pub mod projection;
