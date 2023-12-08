#pragma once

#include <type_traits>

// Convert enum class item to its value.
// https://stackoverflow.com/a/45632890
template <typename T> consteval std::underlying_type_t<T> get_idx(T value) {
  return static_cast<std::underlying_type_t<T>>(value);
}
