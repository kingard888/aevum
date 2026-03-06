// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # BSON/JSON Value Comparison Logic
//!
//! This module provides the critical functionality for comparing `serde_json::Value` instances,
//! establishing a consistent and total ordering across all JSON types. This capability is
//! fundamental to the correct implementation of sorting operations within the AevumDB query engine.
//! The comparison logic rigorously adheres to a predefined type precedence hierarchy, ensuring
//! deterministic and predictable outcomes when comparing values of heterogeneous types.

use serde_json::Value;
use std::cmp::Ordering;

/// Assigns a numerical precedence value to a `serde_json::Value` based on its type.
///
/// This internal helper function is central to establishing a stable sort order when comparing
/// values of disparate types. It defines an arbitrary but consistent type hierarchy, which is a
/// prerequisite for creating a total ordering over the set of all possible JSON values.
///
/// The defined precedence order is as follows:
/// `Null` < `Bool` < `Number` < `String` < `Array` < `Object`.
fn get_type_precedence(value: &Value) -> u8 {
    match value {
        Value::Null => 0,
        Value::Bool(_) => 1,
        Value::Number(_) => 2,
        Value::String(_) => 3,
        Value::Array(_) => 4,
        Value::Object(_) => 5,
    }
}

/// Compares two `serde_json::Value` instances to determine their canonical relative order.
///
/// This function is the cornerstone of all sorting operations within the query engine. It provides
/// a robust and consistent total ordering for all JSON types by first comparing their type
/// precedence. If the types are identical, it then proceeds to a value-specific comparison.
///
/// # Comparison Semantics
///
/// The function implements the following detailed comparison rules:
///
/// - **Type Precedence**: If the types of `a` and `b` differ, their ordering is determined by the
///   pre-defined hierarchy: `Null` < `Bool` < `Number` < `String` < `Array` < `Object`.
///
/// - **Value-Specific Comparison (for identical types)**:
///   - `Null`: All `Null` values are considered `Equal`.
///   - `Bool`: Standard boolean ordering is applied, where `false` is less than `true`.
///   - `Number`: Numerical values are compared as `f64` floating-point numbers. A special case
///     handles `NaN` values by treating them as `Equal` to each other to ensure a stable sort,
///     as `f64`'s `partial_cmp` would otherwise return `None`.
///   - `String`: Standard lexicographical (byte-order) comparison is performed.
///   - `Array` & `Object`: In this implementation, instances of `Array` and `Object` are always
///     considered `Equal` to other instances of their same type. A more sophisticated,
///     production-grade implementation might perform a deep, recursive comparison of their contents,
///     but this is often computationally expensive and not required for many use cases.
///
/// # Arguments
///
/// * `a` - A reference to the first `serde_json::Value` to be compared.
/// * `b` - A reference to the second `serde_json::Value` to be compared.
///
/// # Returns
///
/// An `Ordering` enum (`Less`, `Equal`, or `Greater`) that precisely describes the relative
/// ordering of value `a` with respect to value `b`.
///
/// # Examples
///
/// ```
/// use serde_json::{json, Value};
/// use std::cmp::Ordering;
/// use aevum_ffi::query::comparator::compare_values;
///
/// // Demonstrating type precedence rules.
/// assert_eq!(compare_values(&json!(null), &json!(1)), Ordering::Less);
/// assert_eq!(compare_values(&json!("hello"), &json!(123)), Ordering::Greater);
///
/// // Demonstrating same-type comparison rules.
/// assert_eq!(compare_values(&json!(false), &json!(true)), Ordering::Less);
/// assert_eq!(compare_values(&json!(20.5), &json!(10.1)), Ordering::Greater);
/// assert_eq!(compare_values(&json!("apple"), &json!("banana")), Ordering::Less);
///
/// // Arrays and Objects of the same type are considered equal in this implementation.
/// assert_eq!(compare_values(&json!([1, 2]), &json!([3])), Ordering::Equal);
/// ```
pub fn compare_values(a: &Value, b: &Value) -> Ordering {
    // The lowest precedence type, Null, is handled first as a distinct case.
    if a.is_null() && b.is_null() {
        return Ordering::Equal;
    }
    if a.is_null() {
        return Ordering::Less;
    }
    if b.is_null() {
        return Ordering::Greater;
    }

    // If types are not identical, their defined precedence determines their order.
    let a_prec = get_type_precedence(a);
    let b_prec = get_type_precedence(b);

    if a_prec != b_prec {
        return a_prec.cmp(&b_prec);
    }

    // If we reach this point, the types are identical; proceed to value-based comparison.
    match (a, b) {
        (Value::Bool(ba), Value::Bool(bb)) => ba.cmp(bb),
        (Value::Number(na), Value::Number(nb)) => {
            // `partial_cmp` is used for floating-point numbers as it correctly handles `NaN`.
            // We unwrap to `Ordering::Equal` for `NaN` vs `NaN` to ensure sort stability,
            // as this is a common requirement for database-like sorting.
            na.as_f64().partial_cmp(&nb.as_f64()).unwrap_or(Ordering::Equal)
        }
        (Value::String(sa), Value::String(sb)) => sa.cmp(sb),

        // For composite types, this simplified implementation considers them equal.
        // A production-level system might implement a deep, recursive comparison,
        // but this is often complex and computationally intensive.
        (Value::Array(_), Value::Array(_)) => Ordering::Equal,
        (Value::Object(_), Value::Object(_)) => Ordering::Equal,

        // This catch-all case should theoretically be unreachable if the type precedence
        // logic is exhaustive and all `Value` variants are handled. It ensures function totality.
        _ => Ordering::Equal,
    }
}
