#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/transition/transition.h"
#include "state_machine/transition/transition_row.h"

#include <tuple>
#include <utility>

namespace state_machine {
namespace transition {

using ::state_machine::containers::index_map;
using ::state_machine::containers::list;
namespace op = ::state_machine::containers::op;

template <class R, class... Rs>
class Table;

template <class R, class... Rs>
constexpr auto make_table(R&& first, Rs&&... others) noexcept -> Table<R, Rs...> {
    static_assert(is_row<R>::value, "A `Table` must be composed of `Row`s.");
    static_assert(stdx::conjunction<is_row<Rs>...>::value, "A `Table` must be composed of `Row`s.");

    return Table<R, Rs...>{std::forward<R>(first), std::forward<Rs>(others)...};
}

namespace detail {

template <class Tb>
constexpr auto from_transition_args_impl(Tb&& table) noexcept {
    return std::forward<Tb>(table);
}

template <class Tb, class A1, class A2, class A3, class A4, class A5, class... Ts>
constexpr auto from_transition_args_impl(
    Tb&& table, A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, Ts&&... args) noexcept {
    return from_transition_args_impl(
        std::forward<Tb>(table).update(make_transition(std::forward<A1>(a1),
                                                       std::forward<A2>(a2),
                                                       std::forward<A3>(a3),
                                                       std::forward<A4>(a4),
                                                       std::forward<A5>(a5))),
        std::forward<Ts>(args)...);
}

} // namespace detail

template <class A1, class A2, class A3, class A4, class A5, class... Ts>
constexpr auto make_table_from_transition_args(
    A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5, Ts&&... args) noexcept {
    constexpr std::size_t num_elems_in_row = 5;
    static_assert(sizeof...(Ts) % num_elems_in_row == 0,
                  "The number of args must be a multiple of 5.");

    return detail::from_transition_args_impl(
        make_table(make_row(make_transition(std::forward<A1>(a1),
                                            std::forward<A2>(a2),
                                            std::forward<A3>(a3),
                                            std::forward<A4>(a4),
                                            std::forward<A5>(a5)))),
        std::forward<Ts>(args)...);
}

namespace detail {

template <class T>
struct get_key {
    using type = std::pair<typename T::first_type::key_type, typename T::second_type>;
};

template <class R>
struct get_source {
    using type = typename R::source_type;
};

template <class R>
struct get_event {
    using type = typename R::event_type;
};

template <class R>
struct get_destinations {
    using type = typename R::destination_types;
};

} // namespace detail

template <class R, class... Rs>
class Table {
  public:
    using type = Table<R, Rs...>;
    using row_index_map = op::map<detail::get_key, index_map<R, Rs...>>;
    using event_types = op::make_unique<op::map<detail::get_event, list<R, Rs...>>>;
    using state_types = op::make_unique<
        op::flatten<list<op::map<detail::get_source, list<R, Rs...>>,
                         op::flatten<op::map<detail::get_destinations, list<R, Rs...>>>>>>;
    using data_type = std::tuple<R, Rs...>;
    static constexpr size_t size = 1 + sizeof...(Rs);

    constexpr explicit Table(R&& first, Rs&&... others) noexcept
        : data_{std::make_tuple(std::forward<R>(first), std::forward<Rs>(others)...)} {}

    constexpr auto data() const noexcept -> const data_type& { return data_; }

    constexpr auto into_data() && noexcept -> data_type&& { return std::move(data_); }

    template <class T>
    constexpr auto update(T&& transition) && noexcept {
        static_assert(is_transition<T>::value, "Argument to `update` must be a `Transition`.");

        return std::move(*this).update_impl(std::forward<T>(transition),
                                            std::make_index_sequence<size>{});
    }

  private:
    template <class T,
              size_t... Is,
              std::enable_if_t<!row_index_map::template contains_key<typename T::key_type>::value,
                               int> = 0>
    constexpr auto update_impl(T&& transition, std::index_sequence<Is...>) && noexcept {
        return make_table(std::get<Is>(std::move(*this).into_data())...,
                          make_row(std::forward<T>(transition)));
    }

    template <class T,
              size_t... Is,
              std::enable_if_t<row_index_map::template contains_key<typename T::key_type>::value,
                               int> = 0>
    constexpr auto update_impl(T&& transition, std::index_sequence<Is...>) && noexcept {
        constexpr size_t Index = row_index_map::template at_key<typename T::key_type>::value;

        return std::move(*this).template update_table_row<T, Index>(
            std::forward<T>(transition),
            std::make_index_sequence<Index>{},
            aux::make_index_range<Index + 1, size>{});
    }

    template <class T, size_t Index, size_t... Left, size_t... Right>
    constexpr auto update_table_row(T&& transition,
                                    std::index_sequence<Left...>,
                                    std::index_sequence<Right...>) && noexcept {
        auto data = std::move(*this).into_data();

        // Each call to `std::get` only removes the "Left", "Index", or "Right" elements from
        // `data_`, leaving other elements to other calls of `std::get`.

        // NOLINTNEXTLINE(bugprone-use-after-move)
        return make_table(std::get<Left>(std::move(data))...,
                          // NOLINTNEXTLINE(bugprone-use-after-move)
                          std::get<Index>(std::move(data)).append(std::forward<T>(transition)),
                          // NOLINTNEXTLINE(bugprone-use-after-move)
                          std::get<Right>(std::move(data))...);
    }

    std::tuple<R, Rs...> data_;
};

template <class T>
using is_table = aux::is_specialization_of<Table, T>;

} // namespace transition
} // namespace state_machine
