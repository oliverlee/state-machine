#pragma once

#include <type_traits>

namespace state_machine {
namespace containers {
// Type containers

template <class T> struct identity { using type = T; };

template <class...> struct list { using type = list; };

template <class... Ts> struct inheritor : Ts... { using type = inheritor; };

namespace op {
// Functions to operate on type containers

namespace detail {

template <class K, class... Ts> struct contains_impl;

template <class K, template <class...> class L, class... Ts>
struct contains_impl<K, L<Ts...>> : contains_impl<K, Ts...> {};

template <class K, class... Ts> struct contains_impl {
    using type = std::is_base_of<K, inheritor<Ts...>>;
};

} // namespace detail

// Check if type K is contained within L<Ts...>.
template <class K, class L>
using contains = typename detail::contains_impl<K, L>::type;

namespace detail {

template <class L> struct make_unique_impl_input;
template <class L, class... Ts> struct make_unique_impl;

template <template <class...> class L, class... Ts>
struct make_unique_impl_input<L<Ts...>> : make_unique_impl<L<>, Ts...> {};

template <template <class...> class L, class... Rs, class T, class... Ts>
struct make_unique_impl<L<Rs...>, T, Ts...>
    : std::conditional_t<contains<T, L<Rs...>>::value,
                         make_unique_impl<L<Rs...>, Ts...>,
                         make_unique_impl<L<Rs..., T>, Ts...>> {};

template <template <class...> class L, class... Rs>
struct make_unique_impl<L<Rs...>> {
    using type = L<Rs...>;
};

} // namespace detail

// Given L<Ts...>, return L<Rs...> where Rs... are the unique elements of Ts...
template <class L>
using make_unique = typename detail::make_unique_impl_input<L>::type;

namespace detail {

template <template <class> class Pred, class L> struct filter_impl_input;
template <template <class> class Pred, class L, class... Ts> struct filter_impl;

template <template <class> class Pred, template <class...> class L, class... Ts>
struct filter_impl_input<Pred, L<Ts...>> : filter_impl<Pred, L<>, Ts...> {};

template <template <class> class Pred, template <class...> class L, class... Rs,
          class T, class... Ts>
struct filter_impl<Pred, L<Rs...>, T, Ts...>
    : std::conditional_t<Pred<T>::value, filter_impl<Pred, L<Rs..., T>, Ts...>,
                         filter_impl<Pred, L<Rs...>, Ts...>> {};

template <template <class> class Pred, template <class...> class L, class... Rs>
struct filter_impl<Pred, L<Rs...>> : L<Rs...> {};

} // namespace detail

// Given L<Ts...>, return L<Rs...> where Rs... are the elements of Ts... that
// satisfy Pred
template <template <class> class Pred, class L>
using filter = typename detail::filter_impl_input<Pred, L>::type;

} // namespace op
} // namespace containers
} // namespace state_machine
