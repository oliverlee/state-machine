#pragma once

#include "state_machine/backport.h"
#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/transition.h"
#include "state_machine/transition_row.h"

#include <tuple>
#include <utility>

namespace state_machine {
namespace transition {
using ::state_machine::containers::index_map;
namespace op = ::state_machine::containers::op;

template <class R, class... Rs>
class Table;

template <class R, class... Rs>
constexpr auto make_table(R&& first, Rs&&... others) noexcept {
    static_assert(is_row<R>::value, "A `Table` must be composed of `Row`s.");
    static_assert(stdx::conjunction<is_row<Rs>...>::value, "A `Table` must be composed of `Row`s.");

    return Table<R, Rs...>(std::forward<R>(first), std::forward<Rs>(others)...);
}

namespace detail {

template <class T>
struct extract_key {
    using type = std::pair<typename T::first_type::key_type, typename T::second_type>;
};

} // namespace detail

template <class R, class... Rs>
class Table {
  public:
    using type = Table<R, Rs...>;
    using row_index_map = op::map<detail::extract_key, index_map<R, Rs...>>;
    using data_type = std::tuple<R, Rs...>;
    static constexpr size_t size = 1 + sizeof...(Rs);

    constexpr Table(R&& first, Rs&&... others) noexcept
        : data_{std::make_tuple(std::forward<R>(first), std::forward<Rs>(others)...)} {}

    constexpr auto data() const noexcept -> const data_type& { return data_; }

    constexpr auto into_data() && noexcept -> data_type&& { return std::move(data_); }

    template <class T>
    constexpr auto update(T&& transition) && noexcept {
        static_assert(is_transition<T>::value, "Argument to `update` must be a `Transition`.");

        return std::move(*this).update_impl(std::move(transition),
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
            std::make_index_sequence<size - Index - 1>{});
    }

    template <class T, size_t Index, size_t... Left, size_t... Right>
    constexpr auto update_table_row(T&& transition,
                                    std::index_sequence<Left...>,
                                    std::index_sequence<Right...>) && noexcept {
        auto data = std::move(*this).into_data();
        return make_table(std::get<Left>(std::move(data))...,
                          std::get<Index>(std::move(data)).append(std::forward<T>(transition)),
                          std::get<Right + Index + 1>(std::move(data))...);
    }

    std::tuple<R, Rs...> data_;
};

template <class T>
using is_table = aux::is_specialization_of<Table, T>;

} // namespace transition
} // namespace state_machine
