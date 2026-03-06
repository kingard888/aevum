// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Core Document Matching and Filtering Logic
//!
//! This module provides the central logic for determining whether a given document satisfies a
//! specified query. It is a foundational component of the AevumDB query engine, responsible for
//! filtering documents based on a wide array of criteria. The matching logic supports both direct,
//! field-level equality checks and complex, operator-driven conditions (e.g., `$gt`, `$lt`).
//!
//! A key design principle of this module is its robustness. The matching process is engineered to
//! handle structural divergences between documents and queries gracefully. For instance, if a query
//! references a field that does not exist in a document, the condition simply evaluates to `false`
//! rather than causing a panic or an error, which is the correct and expected behavior for a
//! document-oriented database.

use super::operators;
use serde_json::Value;

/// Recursively evaluates if a document (`doc`) satisfies all conditions within a `query`.
///
/// This function is the primary entry point for the query matching engine. It systematically
/// iterates through the key-value pairs of the `query` object and evaluates them against the
/// corresponding fields in the `doc`. The function supports nested documents and a rich set of
/// query operators.
///
/// ## Matching Logic
///
/// The evaluation adheres to the following rules, which are consistent with MongoDB's query semantics:
///
/// - **Implicit `AND`**: All top-level key-value pairs in the query object are implicitly joined
///   by a logical `AND`. The document must satisfy every condition to be considered a match.
///
/// - **Empty or Null Query**: An empty query object (`{}`) or a `null` query value acts as a
///   wildcard, matching all documents.
///
/// - **Missing Fields**: If a query specifies a condition on a field that does not exist within the
///   `doc`, that specific condition evaluates to `false`, and therefore the document does not match.
///
/// - **Operator-Based Queries**: If a query value for a given key is a JSON object, and the first key
///   within that object begins with a `$` character, it is interpreted as an operator expression
///   (e.g., `{"age": {"$gt": 30}}`). The evaluation is then delegated to the `operators::evaluate`
///   function, which handles the specific logic for operators like `$gt`, `$lt`, etc.
///
/// - **Direct Equality**: If a query value is not an operator expression, a direct equality
///   comparison (`==`) is performed between the document's field value and the query's value.
///   This includes comparisons of nested documents and arrays.
///
/// # Arguments
///
/// * `doc` - A reference to the `serde_json::Value` representing the document to be evaluated.
/// * `query` - A reference to the `serde_json::Value` representing the query object.
///
/// # Returns
///
/// Returns `true` if the `doc` is determined to be a complete match for the `query`; otherwise,
/// returns `false`.
///
/// # Examples
///
/// ```
/// use serde_json::{json, Value};
/// use aevum_ffi::query::matcher::matches_query;
///
/// let doc = json!({ "name": "Alice", "age": 30, "city": "New York" });
///
/// // An empty query will match any document.
/// assert!(matches_query(&doc, &json!({})));
///
/// // A direct field-value match.
/// assert!(matches_query(&doc, &json!({ "name": "Alice" })));
/// assert!(!matches_query(&doc, &json!({ "name": "Bob" })));
///
/// // An operator-based match (`$gt` for "greater than").
/// assert!(matches_query(&doc, &json!({ "age": { "$gt": 25 } })));
/// assert!(!matches_query(&doc, &json!({ "age": { "$gt": 35 } })));
///
/// // A query with multiple conditions (implicit AND).
/// assert!(matches_query(&doc, &json!({ "city": "New York", "age": { "$lt": 35 } })));
///
/// // A query on a field that does not exist in the document.
/// assert!(!matches_query(&doc, &json!({ "state": "NY" })));
/// ```
pub fn matches_query(doc: &Value, query: &Value) -> bool {
    // A null or empty query is treated as a universal match. This is a standard convention.
    if query.is_null() || (query.is_object() && query.as_object().unwrap().is_empty()) {
        return true;
    }

    if let Some(q_obj) = query.as_object() {
        // All conditions in the query object must be satisfied (implicit AND semantics).
        for (key, q_val) in q_obj {
            // Safely retrieve the value from the document corresponding to the query key.
            // If the key does not exist in the document, it cannot satisfy the condition.
            let doc_val = match doc.get(key) {
                Some(v) => v,
                None => return false,
            };

            if let Some(operator_map) = q_val.as_object() {
                // Heuristically determine if a nested object represents an operator map.
                // This is done by checking if the first key in the object starts with '$'.
                let is_operator_query =
                    operator_map.keys().next().map_or(false, |k| k.starts_with('$'));

                if is_operator_query {
                    // If it is an operator query, evaluate each operator within the map.
                    for (op, target) in operator_map {
                        if !operators::evaluate(op, doc_val, target) {
                            return false; // If any single operator condition fails, the entire document fails to match.
                        }
                    }
                } else if doc_val != q_val {
                    // This is not an operator map, but a nested object. Perform a deep equality check.
                    return false;
                }
            } else if doc_val != q_val {
                // This is a direct primitive value comparison. If they are not equal, the match fails.
                return false;
            }
        }
    } else {
        // If the query itself is not an object (e.g., just a string or number), the match
        // only succeeds if the entire document is equal to that primitive value. This is an
        // edge case but is handled for completeness.
        return doc == query;
    }
    // If the function has not returned `false` after checking all conditions, it means
    // the document is a valid match.
    true
}
