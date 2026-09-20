#pragma once

namespace enzo::expr {

/// @brief The module name a script requires for the vector operators.
inline constexpr const char* vectorOperatorsModule = "enzo_vector";

/**
 * @brief Arithmetic between a float3 and a single number.
 *
 * @note daslang defines only multiply and divide by a float, so the rest are
 * defined here along with every form taking an int.
 */
inline constexpr const char* vectorOperatorsSource = R"das(
options gen2

module enzo_vector shared

def public operator + (a : float3; b : float) : float3 { return a + float3(b) }
def public operator + (a : float; b : float3) : float3 { return float3(a) + b }
def public operator - (a : float3; b : float) : float3 { return a - float3(b) }
def public operator - (a : float; b : float3) : float3 { return float3(a) - b }

def public operator += (var a : float3&; b : float) { a += float3(b) }
def public operator -= (var a : float3&; b : float) { a -= float3(b) }

def public operator + (a : float3; b : int) : float3 { return a + float(b) }
def public operator + (a : int; b : float3) : float3 { return float(a) + b }
def public operator - (a : float3; b : int) : float3 { return a - float(b) }
def public operator - (a : int; b : float3) : float3 { return float(a) - b }
def public operator * (a : float3; b : int) : float3 { return a * float(b) }
def public operator * (a : int; b : float3) : float3 { return float(a) * b }
def public operator / (a : float3; b : int) : float3 { return a / float(b) }
def public operator / (a : int; b : float3) : float3 { return float(a) / b }

def public operator += (var a : float3&; b : int) { a += float(b) }
def public operator -= (var a : float3&; b : int) { a -= float(b) }
def public operator *= (var a : float3&; b : int) { a *= float(b) }
def public operator /= (var a : float3&; b : int) { a /= float(b) }
)das";

} // namespace enzo::expr
