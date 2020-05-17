#pragma once

#include "state_machine/backport.h"

#include <cstddef>
#include <type_traits>

namespace state_machine {
namespace aux {
// General purpose extensions to type_traits

// Check if a type is a template specialization
template <template <class...> class Template, class T>
struct is_specialization_of : std::false_type {};
template <template <class...> class Template, class... Args>
struct is_specialization_of<Template, Template<Args...>> : std::true_type {};

// Check if a type is copy or move constructible
template <class T>
using is_copy_or_move_constructible =
    stdx::disjunction<std::is_copy_constructible<T>, std::is_move_constructible<T>>;

// A convenience type alias for working an index_map.
template <std::size_t I>
using index_constant = std::integral_constant<std::size_t, I>;


// https://stackoverflow.com/questions/20874388/error-spliting-an-stdindex-sequence
namespace detail {

template <typename T, typename Seq, T Begin>
struct make_integer_range_impl;

template <typename T, T... Ints, T Begin>
struct make_integer_range_impl<T, std::integer_sequence<T, Ints...>, Begin> {
    using type = std::integer_sequence<T, Begin + Ints...>;
};

} // namespace detail

/* Similar to std::make_integer_sequence<>, except it goes from [Begin, End)
   instead of [0, End). */
template <typename T, T Begin, T End>
using make_integer_range = typename detail::
    make_integer_range_impl<T, std::make_integer_sequence<T, End - Begin>, Begin>::type;

/* Similar to std::make_index_sequence<>, except it goes from [Begin, End)
   instead of [0, End). */
template <std::size_t Begin, std::size_t End>
using make_index_range = make_integer_range<std::size_t, Begin, End>;

} // namespace aux
} // namespace state_machine
