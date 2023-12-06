#pragma once

// Convert enum class item to its value.
// https://stackoverflow.com/a/45632890
template <typename T>
consteval auto get_idx(T value)
{
    return static_cast<std::underlying_type_t<T>>(value);
}
