#pragma once

#include <type_traits>

namespace state_machine {
namespace stdx {

// Backport std::conjunction (C++17)
// https://en.cppreference.com/w/cpp/experimental/conjunction#Possible_implementation
template <class...>
struct conjunction : std::true_type {};
template <class B1>
struct conjunction<B1> : B1 {};
template <class B1, class... Bn>
struct conjunction<B1, Bn...> : std::conditional_t<bool(B1::value), conjunction<Bn...>, B1> {};

// Backport std::disjunction (C++17)
// https://en.cppreference.com/w/cpp/experimental/disjunction#Possible_implementation
template <class...>
struct disjunction : std::false_type {};
template <class B1>
struct disjunction<B1> : B1 {};
template <class B1, class... Bn>
struct disjunction<B1, Bn...> : std::conditional_t<bool(B1::value), B1, disjunction<Bn...>> {};

// Backport std::negation (C++17)
// https://en.cppreference.com/w/cpp/types/negation#Possible_implementation
template <class B>
struct negation : std::integral_constant<bool, !bool(B::value)> {};

// Backport std::void_t (C++17)
template <typename...>
using void_t = void;

// Backport std::bool_constant (C++17)
template <bool B>
using bool_constant = std::integral_constant<bool, B>;

} // namespace stdx
} // namespace state_machine
