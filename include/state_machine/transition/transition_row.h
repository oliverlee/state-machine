#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/optional.h"
#include "state_machine/traits.h"
#include "state_machine/transition/transition.h"

#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace state_machine {
namespace transition {

using ::state_machine::containers::list;
using ::state_machine::optional::optional;
namespace op = ::state_machine::containers::op;

namespace detail {

template <class T>
struct get_destination {
    using type = typename T::destination_type;
};

template <class T>
using is_not_void = stdx::negation<std::is_void<T>>;

} // namespace detail

template <class T, class... Ts>
class Row {
  public:
    static_assert(is_transition<T>::value, "A `Row` must be composed of `Transition`s.");
    static_assert(stdx::conjunction<is_transition<Ts>...>::value,
                  "A `Row` must be composed of `Transition`s.");
    static_assert(
        stdx::conjunction<std::is_same<typename T::key_type, typename Ts::key_type>...>::value,
        "All `Transition`s in a `Row` must have the same `Key`.");

    using type = Row<T, Ts...>;
    using key_type = typename T::key_type;
    using source_type = typename T::source_type;
    using event_type = typename T::event_type;
    using destination_types =
        op::filter<detail::is_not_void,
                   op::make_unique<op::map<detail::get_destination, list<T, Ts...>>>>;
    using data_type = std::tuple<T, Ts...>;
    static constexpr size_t size = 1 + sizeof...(Ts);

    constexpr explicit Row(T&& first, Ts&&... others) noexcept
        : data_{std::make_tuple(std::forward<T>(first), std::forward<Ts>(others)...)} {}

    constexpr auto data() const noexcept -> const data_type& { return data_; }

    constexpr auto into_data() && noexcept -> data_type&& { return std::move(data_); }

    template <class U>
    constexpr auto append(U&& transition) && noexcept {
        static_assert(is_transition<U>::value, "A `Row` must be composed of `Transition`s.");

        return std::move(*this).append_impl(std::forward<U>(transition),
                                            std::make_index_sequence<size>{});
    }

    auto find_transition(const source_type& source, const event_type& event) const noexcept(
        stdx::conjunction<is_nothrow_guard_invocable<T>, is_nothrow_guard_invocable<Ts>...>::value)
        -> optional<size_t> {
        return find_transition_impl(source, event, std::make_index_sequence<size>{});
    }

  private:
    template <class U, size_t... Is>
    constexpr auto append_impl(U&& transition, std::index_sequence<Is...>) && noexcept {
        return Row<T, Ts..., U>(std::get<Is>(std::move(*this).into_data())...,
                                std::forward<U>(transition));
    }

    template <class S, class E>
    constexpr auto find_transition_impl(S, E, std::index_sequence<>) const -> optional<size_t> {
        return {};
    }

    template <class S, class E, size_t I, size_t... Is>
    constexpr auto find_transition_impl(S source, E event, std::index_sequence<I, Is...>) const
        -> optional<size_t> {
        return std::get<I>(data_).invoke_guard(source, event) ?
                   optional<size_t>{I} :
                   find_transition_impl(source, event, std::index_sequence<Is...>{});
    }

    data_type data_;
};


template <class T, class... Ts>
constexpr auto make_row(T&& first, Ts&&... others) noexcept {
    static_assert(is_transition<T>::value, "A `Row` must be composed of `Transition`s.");
    static_assert(stdx::conjunction<is_transition<Ts>...>::value,
                  "A `Row` must be composed of `Transition`s.");

    return Row<T, Ts...>{std::forward<T>(first), std::forward<Ts>(others)...};
}

template <class R>
using is_row = aux::is_specialization_of<Row, R>;

} // namespace transition
} // namespace state_machine
