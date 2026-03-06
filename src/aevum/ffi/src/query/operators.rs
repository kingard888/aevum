// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # BSON/JSON Query Operator Evaluation Logic
//!
//! This module forms the semantic core of the query language, providing the concrete implementation
//! for evaluating a rich set of MongoDB-style query operators against `serde_json::Value` instances.
//! It serves as a central dispatch hub where operator strings (e.g., `"$gt"`) are mapped to specific
//! comparison or type-checking logic.
//!
//! The functions herein are meticulously designed to be resilient to type mismatches, ensuring
//! that comparisons between incompatible types (e.g., a number and an object) gracefully evaluate
//! to `false` rather than causing a panic. This robustness is critical for a schema-flexible
//! document database.
//!
//! ## Currently Implemented Operators
//!
//! - **Comparison Operators**: `$eq` (equal), `$ne` (not equal), `$gt` (greater than),
//!   `$lt` (less than), `$gte` (greater than or equal), `$lte` (less than or equal).
//! - **Element Operators**: `$type` (checks the BSON/JSON type of a field).

use serde_json::Value;

/// Evaluates a specified query operator between a document field and a target value.
///
/// This function acts as the primary dispatcher for all operator-based query conditions. It takes an
/// operator string, the value from the document to be tested (`field_val`), and the value from the
/// query that specifies the condition (`target_val`). It then executes the appropriate logic.
///
/// # Type Coercion and Comparison Rules
///
/// - For relational operators (`$gt`, `$lt`, etc.), the function attempts to perform a meaningful
///   comparison by checking for compatible types. It prioritizes numerical comparison if both
///   operands can be interpreted as `f64`. If not, it attempts a lexicographical comparison if
///   both are strings. If neither is possible, the operation returns `false`.
/// - The `$eq` and `$ne` operators use `serde_json::Value`'s built-in, deep equality comparison.
/// - The `$type` operator validates that the `field_val` matches the BSON/JSON type specified by
///   the `target_val` (which must be a string like `"string"`, `"number"`, etc.).
///
/// # Arguments
///
/// * `op` - A string slice representing the operator (e.g., `"$eq"`, `"$gt"`).
/// * `field_val` - A reference to the `Value` from the document field being evaluated.
/// * `target_val` - A reference to the `Value` from the query, which defines the condition.
///
/// # Returns
///
/// Returns `true` if the `field_val` satisfies the condition specified by the operator and
/// `target_val`; otherwise, returns `false`.
///
/// # Examples
///
/// ```
/// use serde_json::json;
/// use aevum_ffi::query::operators::evaluate;
///
/// // Greater Than (`$gt`) operator on numbers.
/// assert!(evaluate("$gt", &json!(15), &json!(10)));
/// assert!(!evaluate("$gt", &json!(10), &json!(15)));
///
/// // Type check (`$type`) operator.
/// assert!(evaluate("$type", &json!("hello world"), &json!("string")));
/// assert!(!evaluate("$type", &json!(123), &json!("string")));
/// ```
pub fn evaluate(op: &str, field_val: &Value, target_val: &Value) -> bool {
    match op {
        "$eq" => field_val == target_val,
        "$ne" => field_val != target_val,

        "$gt" => {
            if let (Some(d), Some(t)) = (field_val.as_f64(), target_val.as_f64()) {
                d > t
            } else if let (Some(d), Some(t)) = (field_val.as_str(), target_val.as_str()) {
                d > t
            } else {
                false
            }
        }
        "$lt" => {
            if let (Some(d), Some(t)) = (field_val.as_f64(), target_val.as_f64()) {
                d < t
            } else if let (Some(d), Some(t)) = (field_val.as_str(), target_val.as_str()) {
                d < t
            } else {
                false
            }
        }
        "$gte" => {
            if let (Some(d), Some(t)) = (field_val.as_f64(), target_val.as_f64()) {
                d >= t
            } else if let (Some(d), Some(t)) = (field_val.as_str(), target_val.as_str()) {
                d >= t
            } else {
                false
            }
        }
        "$lte" => {
            if let (Some(d), Some(t)) = (field_val.as_f64(), target_val.as_f64()) {
                d <= t
            } else if let (Some(d), Some(t)) = (field_val.as_str(), target_val.as_str()) {
                d <= t
            } else {
                false
            }
        }
        "$type" => {
            if let Some(target_type_str) = target_val.as_str() {
                match target_type_str {
                    "string" => field_val.is_string(),
                    "number" | "int" | "float" | "double" => field_val.is_number(),
                    "boolean" | "bool" => field_val.is_boolean(),
                    "array" => field_val.is_array(),
                    "object" => field_val.is_object(),
                    "null" => field_val.is_null(),
                    _ => false, // An unknown type string results in a failed match.
                }
            } else {
                // The `$type` operator requires its target value to be a string.
                false
            }
        }
        // Any operator not explicitly recognized is treated as a non-match.
        _ => false,
    }
}
