#pragma once

#include "state_machine/containers.h"
#include "state_machine/transition/transition_table.h"
#include "state_machine/variant.h"

#include <tuple>

namespace state_machine {
namespace state_machine {

using ::state_machine::variant::Variant;
namespace op = ::state_machine::containers::op;

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

    template <class... Args>
    StateMachine(Table&& table, Args&&... args) : table_{std::forward<Table>(table)} {
        state_.template emplace<initial_state_type>(std::forward<Args>(args)...);
        state_.visit([](auto&& s) { detail::on_entry(std::forward<decltype(s)>(s)); });
    }

    ~StateMachine() {
        state_.visit([](auto&& s) { detail::on_exit(std::forward<decltype(s)>(s)); });
    }

  private:
    Table table_;
    op::repack<state_types, Variant> state_;
};

} // namespace state_machine
} // namespace state_machine
