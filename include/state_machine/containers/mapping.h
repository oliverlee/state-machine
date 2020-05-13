#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers/basic.h"
#include "state_machine/containers/operations.h"
#include "state_machine/traits.h"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace state_machine {
namespace containers {
namespace mapping {

using state_machine::containers::basic::inheritor;
using state_machine::containers::basic::list;
using state_machine::containers::operations::make_unique;
using state_machine::containers::operations::map;
using state_machine::stdx::conjunction;
using state_machine::stdx::negation;

namespace detail {

template <class T>
struct get_first {
    using type = typename T::first_type;
};

template <class T>
struct get_second {
    using type = typename T::second_type;
};

} // namespace detail

// A mapping where Ts... is a collection of std::pair<key, value> and all keys
// are unique.
template <class... Ts>
struct surjection : inheritor<Ts...> {
    static_assert(conjunction<aux::is_specialization_of<std::pair, Ts>...>::value,
                  "A mapping must be composed of elements of type `std::pair`.");

    using type = surjection<Ts...>;
    using keys = map<detail::get_first, list<Ts...>>;
    using values = make_unique<map<detail::get_second, list<Ts...>>>;

    static_assert(std::is_same<keys, make_unique<keys>>::value,
                  "A mapping cannot contain duplicate keys.");

    template <class, class Default>
    static constexpr auto at_key_impl(...) -> Default;

    template <class Key, class, class Value>
    static constexpr auto at_key_impl(std::pair<Key, Value>*) -> Value;

    template <class Key, class Default = void>
    using at_key =
        decltype(surjection::at_key_impl<Key, Default>(std::declval<inheritor<Ts...>*>()));

    template <class Key>
    using contains_key = negation<std::is_same<void, at_key<Key>>>;
};


// A mapping where Ts... is a collection of std::pair<key, value> and all keys
// are unique and all values are unique.
template <class... Ts>
struct bijection : surjection<Ts...> {
    using type = bijection<Ts...>;
    using keys = map<detail::get_first, list<Ts...>>;
    using values = map<detail::get_second, list<Ts...>>;

    static_assert(std::is_same<values, make_unique<values>>::value,
                  "A bijection cannot contain duplicate values.");

    template <class, class Default>
    static constexpr auto at_value_impl(...) -> Default;

    template <class Value, class, class Key>
    static constexpr auto at_value_impl(std::pair<Key, Value>*) -> Key;

    template <class Value, class Default = void>
    using at_value =
        decltype(bijection::at_value_impl<Value, Default>(std::declval<inheritor<Ts...>*>()));

    template <class Value>
    using contains_value = negation<std::is_same<void, at_value<Value>>>;
};


namespace detail {

template <class... Ts>
struct index_map_impl;

template <class... Ts, class K, class... Ks>
struct index_map_impl<list<Ts...>, K, Ks...>
    : index_map_impl<list<Ts..., std::pair<K, aux::index_constant<sizeof...(Ts)>>>, Ks...> {};

template <class... Ts>
struct index_map_impl<list<Ts...>> : bijection<Ts...> {};

} // namespace detail

// A mapping where Ks... is a collection of keys, which are unique. A unique
// value (an index) is assigned to each key.
template <class... Ks>
using index_map = typename detail::index_map_impl<list<>, Ks...>::type;

} // namespace mapping
} // namespace containers
} // namespace state_machine
