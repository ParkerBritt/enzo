// Shared stateless helpers for QML views. Imported with a namespace, e.g.
//   import "../Utils.js" as Utils
//   Utils.clamp(value, lo, hi)
.pragma library

// Returns value confined to the range [lo, hi].
function clamp(value, lo, hi) {
    return Math.min(Math.max(value, lo), hi);
}
