#pragma once

#include "state_machine/containers.h"
#include "state_machine/traits.h"
#include "state_machine/transition/transition_table.h"
#include "state_machine/variant.h"

#include <cstdint>
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

// The return type for `StateMachine::process_event`.
enum process_status : uint8_t { Completed, EventIgnored, GuardFailure, UndefinedTransition };

template <class Table>
class StateMachine {
    static_assert(transition::is_table<std::decay_t<Table>>::value,
                  "A `StateMachine` must be created from a `Table`");

    static_assert(std::conditional_t<std::is_lvalue_reference<Table>::value,
                                     std::is_const<std::remove_reference_t<Table>>,
                                     std::true_type>::value,
                  "When using a Table reference, that Table must be const.");

  public:
    using type = StateMachine<Table>;
    using state_types = typename std::decay_t<Table>::state_types;
    using event_types = typename std::decay_t<Table>::event_types;
    using initial_state_type =
        typename std::tuple_element_t<0, typename std::decay_t<Table>::data_type>::source_type;

    template <class... Args>
    StateMachine(Table&& table, Args&&... args) : table_{std::forward<Table>(table)} {
        state_.template emplace<initial_state_type>(std::forward<Args>(args)...);
        state_.visit([](auto&& s) { detail::on_entry(std::forward<decltype(s)>(s)); });
    }

    StateMachine(StateMachine&&) = default;
    auto operator=(StateMachine &&) -> StateMachine& = default;

    ~StateMachine() {
        state_.visit([](auto&& s) { detail::on_exit(std::forward<decltype(s)>(s)); });
    }

    template <class Event, std::enable_if_t<op::contains<Event, event_types>::value, int> = 0>
    auto process_event(Event&& event) -> process_status {
        static_assert(variant_type::template alternative_index<variant::empty>() == 0, "");

        if (state_.index() == 0) {
            throw bad_state_access{};
        }

        // Skip the first "empty" index when searching for the transition row.
        return find_row(std::forward<Event>(event),
                        aux::make_index_range<1, 1 + variant_type::size>{});
    }

    template <class State, std::enable_if_t<op::contains<State, state_types>::value, int> = 0>
    constexpr auto is_state() -> bool {
        return state_.template holds<State>();
    }

  private:
    using variant_type = op::repack<state_types, Variant>;
    using index_type = typename variant_type::index_type;

    struct transition_not_found {};

    template <class Event>
    constexpr auto find_row(Event&&, std::index_sequence<>) -> process_status {
        return process_status::UndefinedTransition;
    }

    template <class Event, size_t I, size_t... Is>
    constexpr auto find_row(Event&& event, std::index_sequence<I, Is...>) -> process_status {
        using state_type =
            typename variant_type::alternative_index_map::template at_value<aux::index_constant<I>>;

        using key_type = transition::Key<transition::State<state_type>, transition::Event<Event>>;

        return (state_.index() == I) ?
                   get_row_transitions(std::forward<Event>(event),
                                       typename std::decay_t<Table>::row_index_map::
                                           template at_key<key_type, transition_not_found>{}) :
                   find_row(std::forward<Event>(event), std::index_sequence<Is...>{});
    }

    template <class Event>
    auto get_row_transitions(Event&&, transition_not_found) -> process_status {
        return process_status::UndefinedTransition;
    }

    template <class Event,
              class RowIndexConstant,
              std::enable_if_t<std::is_same<aux::index_constant<RowIndexConstant::value>,
                                            RowIndexConstant>::value,
                               int> = 0>
    auto get_row_transitions(Event&& event, RowIndexConstant) -> process_status {
        const auto& row = std::get<RowIndexConstant::value>(table_.data());

        return find_transition(row,
                               std::forward<Event>(event),
                               std::make_index_sequence<std::decay_t<decltype(row)>::size>{});
    }

    template <class Row, class Event>
    auto find_transition(const Row&, Event&&, std::index_sequence<>) -> process_status {
        return process_status::GuardFailure;
    }

    template <class Row, class Event, size_t I, size_t... Is>
    auto find_transition(const Row& row, Event&& event, std::index_sequence<I, Is...>)
        -> process_status {
        const auto& transition = std::get<I>(row.data());
        const auto& state = state_.template get<typename Row::source_type>();

        return transition.invoke_guard(state, event) ?
                   do_transition(transition, std::forward<Event>(event)) :
                   find_transition(row, std::forward<Event>(event), std::index_sequence<Is...>{});
    }

    // Perform an internal transition
    template <class Transition, class Event, std::enable_if_t<Transition::internal, int> = 0>
    auto do_transition(const Transition& transition, Event&& event) -> process_status {
        // This may miss manually created actions that are used to ignore events.
        if (std::is_same<typename Transition::action_type,
                         transition::detail::action_pass>::value) {
            return process_status::EventIgnored;
        }

        auto& s = state_.template get<typename Transition::source_type>();

        transition.invoke_action(s, event);

        return process_status::Completed;
    }

    // Perform an external transition
    template <class Transition, class Event, std::enable_if_t<!Transition::internal, int> = 0>
    auto do_transition(const Transition& transition, Event&& event) -> process_status {
        auto& s = state_.template get<typename Transition::source_type>();

        detail::on_exit(s);

        auto& d = state_.set(transition.invoke_action(s, event));

        detail::on_entry(d);

        return process_status::Completed;
    }

    const Table table_;
    variant_type state_;
};

} // namespace state_machine
} // namespace state_machine
