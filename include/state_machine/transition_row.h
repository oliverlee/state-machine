#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/transition.h"

#include <tuple>
#include <type_traits>
#include <utility>

namespace state_machine {
namespace transition {

template <class T>
using is_transition = aux::is_specialization_of<Transition, T>;

template <class T, class... Ts>
struct Row : std::tuple<T, Ts...> {
    static_assert(is_transition<T>::value, "A `Row` must be composed of `Transition`s.");
    static_assert(stdx::conjunction<is_transition<Ts>...>::value,
                  "A `Row` must be composed of `Transition`s.");
    static_assert(
        stdx::conjunction<std::is_same<typename T::key_type, typename Ts::key_type>...>::value,
        "All `Transition`s in a `Row` must have the same `Key`.");

    using type = Row<T, Ts...>;
    using key_type = typename std::decay_t<T>::key_type;
    static constexpr size_t size = 1 + sizeof...(Ts);

    constexpr Row(T&& first, Ts&&... others) noexcept
        : std::tuple<T, Ts...>{
              std::make_tuple(std::forward<T>(first), std::forward<Ts>(others)...)} {}
};


template <class T, class... Ts>
auto constexpr make_row(T&& first, Ts&&... others) noexcept {
    static_assert(is_transition<T>::value, "");
    static_assert(stdx::conjunction<is_transition<Ts>...>::value, "");

    return Row<T, Ts...>(std::forward<T>(first), std::forward<Ts>(others)...);
}

namespace detail {

template <class... Ts, class T, size_t... Is>
auto constexpr update_row_impl(Row<Ts...>&& row,
                               T&& transition,
                               std::index_sequence<Is>...) noexcept {
    return Row<T, Ts...>(std::forward<T>(transition), std::get<Is>(std::move(row))...);
}
} // namespace detail

template <class R, class T>
auto constexpr update_row(R&& row, T&& transition) noexcept {
    static_assert(aux::is_specialization_of<Row, R>::value, "");
    static_assert(is_transition<T>::value, "");

    return detail::update_row_impl(
        std::forward<R>(row), std::forward<T>(transition), std::make_index_sequence<R::size>{});
}

} // namespace transition
} // namespace state_machine
