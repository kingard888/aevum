// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # High-Level Query and Data Manipulation Operations
//!
//! This module serves as the primary orchestrator for high-level database operations, including
//! the full suite of CRUD (Create, Read, Update, Delete) functionalities, as well as validation.
//! It stands at the intersection of the various query engine components, composing the foundational
//! logic from its sibling modules (`matcher`, `comparator`, `projection`) to execute complex data
//! manipulation and retrieval tasks.
//!
//! ## Architectural Role
//!
//! The functions within this module represent the public-facing API of the Rust-native query engine.
//! They are designed to be directly consumed by the FFI layer, which then exposes them to external
//! C/C++ clients. Each function is responsible for parsing input data (typically JSON strings),
//! performing the requested operation in parallel using `rayon`, and serializing the results back
//! into a string format.
//!
//! ## Implementation Philosophy and Caveats
//!
//! The current implementation prioritizes clarity, correctness, and robustness against malformed
//! inputs for the purpose of demonstrating a functional FFI layer. It gracefully handles common
//! error conditions, such as invalid JSON, by returning sensible default values (e.g., empty
//! arrays or `0`).
//!
//! However, it is important to note that this is not a production-grade implementation. A
//! production-ready query engine would incorporate significantly more advanced features, such as:
//! - **Indexing**: For efficient, sub-linear time complexity on lookups.
//! - **Query Planning & Optimization**: To reorder and optimize the execution of complex queries.
//! - **Advanced Schema Validation**: A more comprehensive implementation adhering to the full
//!   JSON Schema specification.
//! - **Transactional Semantics**: To ensure atomicity for write operations.

use rayon::prelude::*;
use serde_json::Value;

use super::comparator::compare_values;
use super::matcher::matches_query;
use super::projection::apply_projection;

/// Validates a document represented as a string against a schema query string.
///
/// This function provides a basic schema validation capability. It parses both the document and
/// the schema into `serde_json::Value` representations. The "schema" is treated as a standard
/// query document, and the core `matches_query` logic is used to determine validity.
///
/// Note: This is a simplified interpretation of schema validation. A production-grade system would
/// employ a dedicated, full-featured JSON Schema validation library.
///
/// # Arguments
///
/// * `doc_str` - A string slice (`&str`) representing the JSON document to be validated.
/// * `schema_str` - A string slice (`&str`) representing the JSON schema query.
///
/// # Returns
///
/// Returns `true` if the document is considered valid according to the schema. Returns `false` if
/// either input string is not valid JSON, or if the document fails to satisfy the schema's conditions.
///
/// # Example
///
/// ```
/// use aevum_ffi::query::operations::validate;
///
/// let doc = r#"{ "name": "Alice", "age": 30 }"#;
/// let schema = r#"{ "age": { "$gt": 25 } }"#; // Schema as a query
/// assert!(validate(doc, schema));
///
/// let invalid_doc = r#"{ "name": "Bob", "age": 20 }"#;
/// assert!(!validate(invalid_doc, schema));
/// ```
pub fn validate(doc_str: &str, schema_str: &str) -> bool {
    let doc: Value = match serde_json::from_str(doc_str) {
        Ok(d) => d,
        Err(_) => return false,
    };
    let schema: Value = match serde_json::from_str(schema_str) {
        Ok(s) => s,
        Err(_) => return false,
    };

    // Use the robust `matches_query` logic to perform validation.
    // This allows for both simple type-checking style "schemas" and
    // complex, operator-based criteria.
    matches_query(&doc, &schema)
}

/// Counts the number of documents in a dataset that satisfy a given query.
///
/// This function leverages parallel processing via the `rayon` crate for high-performance
/// counting. It parses the input JSON strings, then uses a parallel iterator to filter the
/// documents according to the query and counts the results.
///
/// # Arguments
///
/// * `data_str` - A string slice representing a dataset as a JSON array of documents.
/// * `query_str` - A string slice representing the filter query. An empty JSON object `{}`
///   will match and count all documents in the dataset.
///
/// # Returns
///
/// An `i32` representing the total count of matching documents. Returns `0` if the input data
/// is not a valid JSON array, if the query is malformed, or if no documents match.
///
/// # Example
///
/// ```
/// use aevum_ffi::query::operations::count;
///
/// let data = r#"[{"val": 10}, {"val": 20}, {"val": 10}]"#;
/// assert_eq!(count(data, r#"{ "val": 10 }"#), 2);
/// assert_eq!(count(data, r#"{ "val": { "$gt": 15 } }"#), 1);
/// assert_eq!(count(data, "{}"), 3); // Counts all documents.
/// ```
pub fn count(data_str: &str, query_str: &str) -> i32 {
    let data: Value = serde_json::from_str(data_str).unwrap_or(Value::Null);
    let query: Value = serde_json::from_str(query_str).unwrap_or(Value::Null);

    if !data.is_array() || query.is_null() {
        return 0; // Robustly handle invalid input by returning a zero count.
    }

    data.as_array()
        .unwrap() // This is safe due to the `is_array` check above.
        .par_iter()
        .filter(|doc| matches_query(doc, &query))
        .count() as i32
}

/// Finds and retrieves a filtered, sorted, and projected subset of documents from a dataset.
///
/// # Arguments
///
/// * `data_str` - A JSON array of documents.
/// * `query_str` - The filter conditions.
/// * `sort_str` - The sort order specification (e.g., `{"field": 1}` for ascending).
/// * `projection_str` - The field inclusion/exclusion rules.
/// * `limit` - The maximum number of documents to return (`0` for no limit).
/// * `skip` - The number of documents to skip from the beginning of the sorted set.
///
/// # Returns
///
/// A `String` containing the JSON array of the resulting documents. Returns an empty array
/// string `[]` if any input is invalid or if no documents match the criteria.
///
/// # Example
///
/// ```
/// use aevum_ffi::query::operations::find;
///
/// let data = r#"[{"name": "B"}, {"name": "A"}, {"name": "C"}]"#;
/// let query = "{}";
/// let sort = r#"{ "name": 1 }"#; // Sort by name ascending.
/// let projection = "{}";
/// // After sorting, the order is A, B, C. Skip 1 ("A"), then limit to 1 ("B").
/// let result = find(data, query, sort, projection, 1, 1);
/// assert_eq!(result, r#"[{"name":"B"}]"#);
/// ```
pub fn find(
    data_str: &str,
    query_str: &str,
    sort_str: &str,
    projection_str: &str,
    limit: usize,
    skip: usize,
) -> String {
    let data: Value = serde_json::from_str(data_str).unwrap_or(Value::Null);
    let query: Value = serde_json::from_str(query_str).unwrap_or(Value::Null);
    let sort: Value = serde_json::from_str(sort_str).unwrap_or(Value::Null);
    let projection: Value = serde_json::from_str(projection_str).unwrap_or(Value::Null);

    if !data.is_array() || query.is_null() {
        return "[]".to_string();
    }

    // Step 1: Filter the documents in parallel based on the query.
    let mut filtered_docs: Vec<Value> = data
        .as_array()
        .unwrap()
        .par_iter()
        .filter(|doc| matches_query(doc, &query))
        .cloned() // `cloned()` is necessary to get owned `Value`s for sorting and modification.
        .collect();

    // Step 2: Apply sorting if a valid sort specification is provided.
    if let Some(sort_obj) = sort.as_object() {
        if !sort_obj.is_empty() {
            // `par_sort_unstable_by` is used for performance. The sort is "unstable" in that
            // elements that compare as equal are not guaranteed to preserve their original order.
            filtered_docs.par_sort_unstable_by(|a, b| {
                for (key, order_val) in sort_obj {
                    let order = if order_val.as_i64() == Some(1) {
                        std::cmp::Ordering::Less // Ascending sort order.
                    } else if order_val.as_i64() == Some(-1) {
                        std::cmp::Ordering::Greater // Descending sort order.
                    } else {
                        // An invalid sort direction value is ignored.
                        return std::cmp::Ordering::Equal;
                    };

                    // Handle cases where a document may not contain the sort key.
                    // Missing fields are treated as `Null` for consistent sorting.
                    let val_a = a.get(key).unwrap_or(&Value::Null);
                    let val_b = b.get(key).unwrap_or(&Value::Null);

                    let cmp_result = compare_values(val_a, val_b);
                    if cmp_result != std::cmp::Ordering::Equal {
                        // Apply the sort direction (ascending or descending).
                        return if order == std::cmp::Ordering::Less {
                            cmp_result
                        } else {
                            cmp_result.reverse()
                        };
                    }
                }
                // If documents are equal according to all sort criteria, their relative order is not guaranteed.
                std::cmp::Ordering::Equal
            });
        }
    }

    // Step 3: Apply pagination (skip/limit) and projection.
    let final_docs: Vec<Value> = filtered_docs
        .into_par_iter()
        .skip(skip)
        .take(if limit == 0 { usize::MAX } else { limit }) // A limit of 0 means no limit.
        .map(|doc| apply_projection(&doc, &projection))
        .collect();

    serde_json::to_string(&final_docs).unwrap_or_else(|_| "[]".to_string())
}

/// Updates all documents in a dataset that match a given query, respecting an optional schema.
///
/// # Arguments
///
/// * `data_str` - A JSON array of documents.
/// * `query_str` - A query to select which documents to update.
/// * `update_str` - A JSON object where each key-value pair will be set or overwritten
///   in the matched documents.
/// * `schema_str` - An optional JSON schema query to validate documents after update.
///
/// # Returns
///
/// A tuple containing:
/// 1. A `String` containing the JSON representation of the entire dataset after the updates.
/// 2. The number of documents that were actually modified.
pub fn update(
    data_str: &str,
    query_str: &str,
    update_str: &str,
    schema_str: &str,
) -> (String, i32) {
    let data: Value = serde_json::from_str(data_str).unwrap_or(Value::Null);
    let query: Value = serde_json::from_str(query_str).unwrap_or(Value::Null);
    let update_doc: Value = serde_json::from_str(update_str).unwrap_or(Value::Null);
    let schema: Value = serde_json::from_str(schema_str).unwrap_or(Value::Null);

    if !data.is_array() || query.is_null() || !update_doc.is_object() {
        return ("[]".to_string(), 0);
    }

    let data_array = data.as_array().unwrap();

    // We use a regular iterator here because we need to count accurately,
    // and simple atomic counters in parallel iterators can be slightly more complex.
    let mut modified_count = 0;
    let updated_docs: Vec<Value> = data_array
        .iter()
        .map(|doc| {
            let mut current_doc = doc.clone();
            if matches_query(&current_doc, &query) {
                let mut candidate_doc = current_doc.clone();
                if let (Some(candidate_obj), Some(update_obj)) =
                    (candidate_doc.as_object_mut(), update_doc.as_object())
                {
                    for (key, val) in update_obj {
                        candidate_obj.insert(key.clone(), val.clone());
                    }
                }

                // Validate the candidate update against the schema if provided.
                if schema.is_null() || matches_query(&candidate_doc, &schema) {
                    current_doc = candidate_doc;
                    modified_count += 1;
                }
            }
            current_doc
        })
        .collect();

    (serde_json::to_string(&updated_docs).unwrap_or_else(|_| "[]".to_string()), modified_count)
}

/// Deletes all documents from a dataset that match a given query.
///
/// # Arguments
///
/// * `data_str` - A JSON array of documents.
/// * `query_str` - A query to select which documents to delete.
///
/// # Returns
///
/// A `String` containing the JSON representation of the dataset after the deletions.
/// Returns `[]` if any input is invalid.
///
/// # Example
///
/// ```
/// use aevum_ffi::query::operations::delete_docs;
///
/// let data = r#"[{"name": "Alice"}, {"name": "Bob"}]"#;
/// let query = r#"{ "name": "Bob" }"#;
/// let result = delete_docs(data, query);
/// assert_eq!(result, r#"[{"name":"Alice"}]"#);
/// ```
pub fn delete_docs(data_str: &str, query_str: &str) -> String {
    let data: Value = serde_json::from_str(data_str).unwrap_or(Value::Null);
    let query: Value = serde_json::from_str(query_str).unwrap_or(Value::Null);

    if !data.is_array() || query.is_null() {
        return "[]".to_string();
    }

    // `filter` is used to retain only the documents that do *not* match the query.
    let remaining_docs: Vec<Value> = data
        .as_array()
        .unwrap()
        .par_iter()
        .filter(|doc| !matches_query(doc, &query))
        .cloned()
        .collect();

    serde_json::to_string(&remaining_docs).unwrap_or_else(|_| "[]".to_string())
}
