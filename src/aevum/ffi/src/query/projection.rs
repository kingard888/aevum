// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # BSON/JSON Document Projection Logic
//!
//! This module provides the core functionality for applying projections to `serde_json::Value`
//! documents. A projection is a powerful feature that allows for the transformation of a document's
//! structure by selectively including or excluding specific fields. This is analogous to the
//! `SELECT` clause in SQL and is essential for enabling clients to shape the data they receive,
//! thereby reducing network overhead and simplifying client-side processing.

use serde_json::{Map, Value};

/// Applies a projection document to a source document, returning a new, transformed document.
///
/// This function constructs a new `serde_json::Value` containing only the fields specified by the
/// projection rules. It adheres to a set of rules inspired by MongoDB's projection semantics,
/// providing a flexible mechanism for data shaping.
///
/// ## Projection Rules and Semantics
///
/// The projection logic is governed by the following principles:
///
/// - **Inclusion Mode**: If the projection document contains any field (other than `_id`) with a
///   "truthy" value (i.e., the number `1` or the boolean `true`), the projection operates in
///   inclusion mode. In this mode, only the fields explicitly marked for inclusion are included
///   in the output document.
///
/// - **Exclusion Mode**: If the projection document contains only fields with "falsy" values
///   (i.e., the number `0` or the boolean `false`), it operates in exclusion mode. In this mode,
///   all fields from the source document are included in the output, *except* for those
///   explicitly marked for exclusion.
///
/// - **The `_id` Field**: By default, the `_id` field is always included in the output, regardless
///   of the mode. It can only be removed by being explicitly excluded in the projection
///   (e.g., `{"_id": 0}`).
///
/// - **Invalid Inputs**: If the `doc` or `projection` arguments are not JSON objects, or if the
///   projection object is empty, the function returns a clone of the original `doc` as a
///   safe fallback.
///
/// # Arguments
///
/// * `doc` - A reference to the original `serde_json::Value` document to be projected.
/// * `projection` - A reference to a `serde_json::Value` object that defines the projection rules.
///
/// # Returns
///
/// A new `serde_json::Value` representing the projected document.
///
/// # Example
///
/// ```
/// use serde_json::json;
/// use aevum_ffi::query::projection::apply_projection;
///
/// let doc = json!({ "_id": "123", "name": "Alice", "age": 30, "details": {"status": "active"} });
///
/// // Inclusion mode: select `name` and `age`. `_id` is included by default.
/// let proj_incl = json!({ "name": 1, "age": 1 });
/// assert_eq!(
///     apply_projection(&doc, &proj_incl),
///     json!({ "_id": "123", "name": "Alice", "age": 30 })
/// );
///
/// // Inclusion mode with explicit `_id` exclusion.
/// let proj_incl_no_id = json!({ "name": 1, "_id": 0 });
/// assert_eq!(
///     apply_projection(&doc, &proj_incl_no_id),
///     json!({ "name": "Alice" })
/// );
///
/// // Exclusion mode: exclude the `details` field.
/// let proj_excl = json!({ "details": 0 });
/// assert_eq!(
///     apply_projection(&doc, &proj_excl),
///     json!({ "_id": "123", "name": "Alice", "age": 30 })
/// );
/// ```
pub fn apply_projection(doc: &Value, projection: &Value) -> Value {
    if let (Some(doc_obj), Some(proj_obj)) = (doc.as_object(), projection.as_object()) {
        if proj_obj.is_empty() {
            return doc.clone(); // An empty projection is a no-op.
        }

        let mut new_obj = Map::new();

        // First, determine if this is an inclusion or exclusion projection.
        // It's inclusion mode if any field other than `_id` is marked with 1 or true.
        let is_inclusion_mode = proj_obj.iter().any(|(key, val)| {
            key != "_id" && (val.as_i64() == Some(1) || val.as_bool() == Some(true))
        });

        if is_inclusion_mode {
            // In inclusion mode, we iterate through the projection spec and add only specified fields.
            for (key, val) in proj_obj {
                if (val.as_i64() == Some(1) || val.as_bool() == Some(true))
                    && doc_obj.contains_key(key)
                {
                    if let Some(doc_field_val) = doc_obj.get(key) {
                        new_obj.insert(key.clone(), doc_field_val.clone());
                    }
                }
            }
            // The `_id` field has special handling: it's always included by default unless explicitly excluded.
            if let Some(id_val) = doc_obj.get("_id") {
                if proj_obj
                    .get("_id")
                    .map_or(true, |v| v.as_i64() != Some(0) && v.as_bool() != Some(false))
                {
                    new_obj.insert("_id".to_string(), id_val.clone());
                }
            }
        } else {
            // In exclusion mode, we iterate through the source document and include all fields
            // unless they are explicitly marked for exclusion in the projection.
            for (key, val) in doc_obj {
                let is_excluded = proj_obj
                    .get(key)
                    .map_or(false, |v| v.as_i64() == Some(0) || v.as_bool() == Some(false));
                if !is_excluded {
                    new_obj.insert(key.clone(), val.clone());
                }
            }
        }

        return Value::Object(new_obj);
    }
    // If the document or projection is not an object, we cannot apply a projection.
    // Return a clone of the original document as a safe fallback.
    doc.clone()
}
