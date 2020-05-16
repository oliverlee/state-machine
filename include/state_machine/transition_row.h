#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/transition.h"

#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace state_machine {
namespace transition {

template <class T>
using is_transition = aux::is_specialization_of<Transition, T>;

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
    using data_type = std::tuple<T, Ts...>;
    static constexpr size_t size = 1 + sizeof...(Ts);

    constexpr Row(T&& first, Ts&&... others) noexcept
        : data_{std::make_tuple(std::forward<T>(first), std::forward<Ts>(others)...)} {}

    constexpr auto data() const noexcept -> const data_type& { return data_; }

    constexpr auto into_data() && noexcept -> data_type&& { return std::move(data_); }

    template <class U>
        constexpr auto append(U&& transition) && noexcept {
        static_assert(is_transition<U>::value, "");

        return std::move(*this).append_impl(std::forward<U>(transition),
                                            std::make_index_sequence<size>{});
    }

    auto find_transition(const source_type& source, const event_type& event) const
        noexcept(stdx::conjunction<has_nothrow_guard<T>, has_nothrow_guard<Ts>...>::value)
            -> size_t {
        return find_transition_impl(*this, source, event, std::make_index_sequence<size>{});
    }

  private:
    template <class U, size_t... Is>
        constexpr auto append_impl(U&& transition, std::index_sequence<Is>...) && noexcept {
        return Row<T, Ts..., U>(std::get<Is>(std::move(*this).into_data())...,
                                std::forward<U>(transition));
    }

    template <class R, class S, class E, size_t I, size_t... Is>
    static constexpr auto
    find_transition_impl(R self, S source, E event, std::index_sequence<I, Is>...) -> size_t {
        return std::get<I>(self.data_).invoke_guard(source, event) ?
                   I :
                   find_transition_impl(self, source, event, std::index_sequence<Is...>{});
    }

    template <class R, class S, class E, size_t I>
    static constexpr auto find_transition_impl(R self, S source, E event, std::index_sequence<I>)
        -> size_t {
        return std::get<I>(self.data_).invoke_guard(source, event) ?
                   I :
                   std::numeric_limits<size_t>::max(); // TODO : replace with optional?
    }

    data_type data_;
};


template <class T, class... Ts>
constexpr auto make_row(T&& first, Ts&&... others) noexcept {
    static_assert(is_transition<T>::value, "");
    static_assert(stdx::conjunction<is_transition<Ts>...>::value, "");

    return Row<T, Ts...>(std::forward<T>(first), std::forward<Ts>(others)...);
}

} // namespace transition
} // namespace state_machine
