#pragma once

#include <cstdint>

// Simple obfuscation stub — returns the value unchanged.
// In the original project this would perform compile-time byte-level
// obfuscation of immediate values used for offsets and string literals.
// The deobfuscated value is identical at runtime so the cheat logic is unaffected.

template <typename T>
constexpr T obfuscatebyte(T v) noexcept { return v; }

// Convenience overload for string literals (used by memory.cpp/scanner.cpp).
inline const char* obfuscatebyte(const char* s) noexcept { return s; }
