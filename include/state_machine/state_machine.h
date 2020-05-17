#pragma once

#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/transition/transition_table.h"
#include "state_machine/variant.h"

#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

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

// The type of exception thrown if `StateMachine::process_event` is called with an invalid internal
// state.
class bad_state_access : public std::exception {};

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

    template <class Event, std::enable_if_t<op::contains<Event, event_types>::value, int> = 0>
    auto process_event(Event &&) -> size_t {
        auto row_index = find_row<Event>(state_.index());

        // TODO: change return type once side effects are tested
        return row_index;
    }

    template <class State, std::enable_if_t<op::contains<State, state_types>::value, int> = 0>
    constexpr auto is_state() -> bool {
        return state_.template holds<State>();
    }

  private:
    using variant_type = op::repack<state_types, Variant>;
    using index_type = typename variant_type::index_type;

    template <class Event>
    constexpr auto find_row(index_type state_index) const -> size_t {
        static_assert(variant_type::template alternative_index<variant::empty>() == 0, "");

        if (state_index == 0) {
            throw bad_state_access{};
        }

        // Skip the first "empty" index when searching for the transition row.
        return find_row_impl<Event>(state_index,
                                    aux::make_index_range<1, 1 + variant_type::size>{});
    }

    template <class Event>
    constexpr auto find_row_impl(index_type, std::index_sequence<>) const -> size_t {
        return std::numeric_limits<size_t>::max(); // TODO: replace with optional
    }

    template <class Event, size_t I, size_t... Is>
    constexpr auto find_row_impl(index_type state_index, std::index_sequence<I, Is...>) const
        -> size_t {

        using state_type =
            typename variant_type::alternative_index_map::template at_value<aux::index_constant<I>>;

        using key_type = transition::Key<transition::State<state_type>, transition::Event<Event>>;

        constexpr size_t row_index =
            std::conditional_t<Table::row_index_map::template contains_key<key_type>::value,
                               typename Table::row_index_map::template at_key<key_type>,
                               aux::index_constant<std::numeric_limits<size_t>::max()>>::value;

        return (state_index == aux::index_constant<I>::value) ?
                   row_index :
                   find_row_impl<Event>(state_index, std::index_sequence<Is...>{});
    }

    const Table table_;

  public: // TODO: make private once side effects are tested
    variant_type state_;
};

} // namespace state_machine
} // namespace state_machine
