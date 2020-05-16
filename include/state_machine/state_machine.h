#pragma once

#include "state_machine/transition/transition_table.h"

#include <tuple>

namespace state_machine {
namespace state_machine {

namespace detail {

template <class, class = void>
struct has_on_entry : std::false_type {};

template <class T>
struct has_on_entry<T, stdx::void_t<decltype(std::declval<T>().on_entry())>> : std::true_type {};

template <class T, std::enable_if_t<!has_on_entry<T>::value, int> = 0>
static auto on_entry(T&) noexcept -> void {}

template <class T, std::enable_if_t<has_on_entry<T>::value, int> = 0>
static auto on_entry(T& t) noexcept(noexcept(std::declval<T>().on_entry())) -> void {
    t.on_entry();
}


template <class, class = void>
struct has_on_exit : std::false_type {};

template <class T>
struct has_on_exit<T, stdx::void_t<decltype(std::declval<T>().on_exit())>> : std::true_type {};

template <class T, std::enable_if_t<!has_on_exit<T>::value, int> = 0>
static auto on_exit(T&) noexcept -> void {}

template <class T, std::enable_if_t<has_on_exit<T>::value, int> = 0>
static auto on_exit(T& t) noexcept(noexcept(std::declval<T>().on_exit())) -> void {
    t.on_exit();
}

} // namespace detail

template <class Table>
class StateMachine {
    static_assert(transition::is_table<Table>::value,
                  "A `StateMachine` must be created from a `Table`");

  public:
    using type = StateMachine<Table>;
    using state_types = typename Table::state_types;
    using event_types = typename Table::event_types;
    using initial_state_type =
        typename std::tuple_element_t<0, typename Table::data_type>::source_type;

    StateMachine(Table&& table) : table_{std::forward<Table>(table)} {}

  private:
    Table table_;
};

} // namespace state_machine
} // namespace state_machine
