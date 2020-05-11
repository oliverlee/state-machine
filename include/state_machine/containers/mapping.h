#pragma once

#include "state_machine/backport.h"
#include "state_machine/traits.h"
#include "state_machine/containers/basic.h"
#include "state_machine/containers/operations.h"
#include <type_traits>
#include <utility>

namespace state_machine {
namespace containers {
namespace mapping {

using state_machine::containers::basic::list;
using state_machine::containers::basic::inheritor;
using state_machine::containers::operations::make_unique;
using state_machine::containers::operations::map;
using state_machine::stdx::conjunction;
using state_machine::stdx::negation;
using state_machine::aux::is_specialization_of;

namespace detail {

template <class T>
struct get_first {
    using type = typename T::first_type;
};

template <class T>
struct get_second {
    using type = typename T::second_type;
};

}

template <class... Ts>
struct surjection : inheritor<Ts...> {
    static_assert(conjunction<is_specialization_of<std::pair, Ts>...>::value, "");

    using type = surjection<Ts...>;
    using keys = map<detail::get_first, list<Ts...>>;
    using values = make_unique<map<detail::get_second, list<Ts...>>>;

    static_assert(std::is_same<keys, make_unique<keys>>::value, "");

    template <class, class Default>
    static constexpr auto at_key_impl(...) -> Default;

    template <class Key, class, class Value>
    static constexpr auto at_key_impl(std::pair<Key, Value>*) -> Value;

    template<class Key, class Default = void>
    using at_key = decltype(at_key_impl<Key, Default>(std::declval<inheritor<Ts...>*>()));

    template<class Key>
    using contains_key = negation<std::is_same<void, at_key<Key>>>;

};

}
}
}
