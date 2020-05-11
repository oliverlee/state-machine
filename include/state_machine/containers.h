#pragma once

#include <type_traits>

namespace state_machine {
namespace containers {
// Type containers

template <class T> struct identity { using type = T; };

template <class...> struct list { using type = list; };

template <class... Ts> struct inherit : Ts... { using type = inherit; };

namespace op {
// Functions to operate on type containers

namespace detail {

template <class K, class... Ts> struct contains_impl;

template <class K, template <class...> class L, class... Ts>
struct contains_impl<K, L<Ts...>> : contains_impl<K, Ts...> {};

template <class K, class... Ts> struct contains_impl {
    using type = std::is_base_of<K, inherit<Ts...>>;
};

} // namespace detail

// Check if type K is contained within L<Ts...>.
template <class K, class L>
using contains = typename detail::contains_impl<K, L>::type;

} // namespace op
} // namespace containers
} // namespace state_machine
