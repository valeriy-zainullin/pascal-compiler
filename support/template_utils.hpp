#pragma once

#include <type_traits> // std::conditional, std::true_type, std::false_type.
#include <variant>

namespace pas {

// https://en.cppreference.com/w/cpp/utility/variant

// https://stackoverflow.com/questions/45892170/how-do-i-check-if-an-stdvariant-can-hold-a-certain-type

template <typename Type, typename... Types>
struct is_one_of : public std::false_type {};

template <typename Type, typename FirstType, typename... OtherTypes>
struct is_one_of<Type, FirstType, OtherTypes...>
    : public std::conditional_t<std::is_same_v<Type, FirstType>, std::true_type,
                                is_one_of<Type, OtherTypes...>> {};

template <typename Type, typename... Types>
using is_one_of_v = is_one_of<Type, Types...>::value;

template <typename Type, typename Variant> struct is_variant_member;

template <typename Type, typename... Types>
struct is_variant_member<Type, std::variant<Types...>>
    : public is_one_of<Type, Types...> {};

// If we have a template, where we will get a value, not a type,
//   we should not use using, but rather define it as a variable!
//   Template variable, how do you like it Ilon Musk?
// Maybe for compile time constants it is the way it is indended to work.
//   I figured out what to do (what to replace "using" with)
//   by looking at std::is_same_v.
//   https://en.cppreference.com/w/cpp/types/is_same
template <typename Type, typename Variant>
inline constexpr bool is_variant_member_v =
    is_variant_member<Type, Variant>::value;

// Check if a type is an instance of a given template type.
//   https://indii.org/blog/is-type-instantiation-of-template/
template <class T, template <class...> class U>
inline constexpr bool is_instance_of_v = std::false_type{};
template <template <class...> class U, class... Vs>
inline constexpr bool is_instance_of_v<U<Vs...>, U> = std::true_type{};
// То есть в объявлениях параметров шаблонов можно указать,
//   что принимается шаблон! Жесть, впервые вижу.

template <typename T, typename U>
inline constexpr bool is_same_nocvref_v =
    std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

} // namespace pas