#pragma once

#include "state_machine/containers.h"
#include "state_machine/traits.h"

#include <type_traits>
#include <utility>

namespace state_machine {
namespace transition {

using ::state_machine::containers::basic::identity;

template <class T>
struct State {
    using type = T;
};

template <class T>
struct Event {
    using type = T;
};

namespace detail {

template <class T>
struct is_state : aux::is_specialization_of<State, std::decay_t<T>> {};

template <class T>
struct is_event : aux::is_specialization_of<Event, std::decay_t<T>> {};

template <class Result, class Callable, class Source, class Event>
using is_transition_callable =
    stdx::conjunction<stdx::negation<std::is_null_pointer<Callable>>,
                      stdx::disjunction<stdx::is_invocable_r<Result, Callable>,
                                        stdx::is_invocable_r<Result, Callable, Event>,
                                        stdx::is_invocable_r<Result, Callable, Source>,
                                        stdx::is_invocable_r<Result, Callable, Source, Event>>>;

template <class T>
using as_action_arg = std::add_lvalue_reference_t<T>;

template <class T>
using as_guard_arg = std::add_const_t<as_action_arg<T>>;

template <class Callable, class Source, class Event>
using is_guard = is_transition_callable<bool, Callable, as_guard_arg<Source>, as_guard_arg<Event>>;

template <class Result, class Callable, class Source, class Event>
using is_action =
    is_transition_callable<Result, Callable, as_action_arg<Source>, as_action_arg<Event>>;

} // namespace detail

template <class Source, class Event, class Guard, class Action, class Destination>
struct Transition : Guard, Action {
    static_assert(detail::is_state<Source>::value,
                  "`Source` type parameter must be a specialization of `State`.");
    static_assert(detail::is_event<Event>::value,
                  "`Event` type parameter must be a specialization of `Event`.");
    static_assert(detail::is_state<Destination>::value,
                  "`Destination` type parameter must be a specialization of `State`.");

    using type = Transition<Source, Event, Guard, Action, Destination>;
    using key_type = std::pair<Source, Event>;
    using source_type = typename Source::type;
    using event_type = typename Event::type;
    using guard_type = Guard;
    using action_type = Action;
    using destination_type = typename Destination::type;
    static constexpr bool internal = std::is_void<destination_type>::value;

    static_assert(
        detail::is_guard<Guard, source_type, event_type>::value,
        "`Guard` type parameter must be callable with `Source` and/or `Event` and return bool.");
    static_assert(detail::is_action<destination_type, Action, source_type, event_type>::value,
                  "`Action` type parameter must be callable with `Source` and/or `Event` and "
                  "return `Destination`.");

    // We should also check if `Guard` is constexpr and returns true,
    // but we assume it to be the case if `Guard` is convertible to a
    // stateless function that takes no arguments and returns a bool.
    static constexpr auto has_empty_guard = std::is_convertible<Guard, bool (*)(void)>::value;

    constexpr Transition(Guard&& guard, Action&& action)
        : Guard{std::forward<Guard>(guard)}, Action{std::forward<Action>(action)} {}

    template <typename... Ts>
    auto guard(Ts&&... ts) const noexcept(noexcept(Guard::operator()(std::forward<Ts>(ts)...)))
        -> bool {
        return Guard::operator()(std::forward<Ts>(ts)...);
    }

    template <typename... Ts>
    auto action(Ts&&... ts) const noexcept(noexcept(Action::operator()(std::forward<Ts>(ts)...)))
        -> destination_type {
        return Action::operator()(std::forward<Ts>(ts)...);
    }
};


namespace detail {

struct empty {};

} // namespace detail

constexpr auto empty_placeholder = identity<detail::empty>{};

namespace detail {

template <class T>
struct is_empty_placeholder
    : std::is_same<std::decay_t<decltype(empty_placeholder)>, std::decay_t<T>> {};

template <class Guard, std::enable_if_t<!is_empty_placeholder<Guard>::value, int> = 0>
auto create_guard_if_empty(Guard&& guard) {
    return std::forward<Guard>(guard);
}

template <class Guard, std::enable_if_t<is_empty_placeholder<Guard>::value, int> = 0>
auto create_guard_if_empty(Guard&&) {
    return [] { return true; };
}

template <class Action, std::enable_if_t<!is_empty_placeholder<Action>::value, int> = 0>
auto create_action_if_empty(Action&& action) {
    return std::forward<Action>(action);
}

template <class Action, std::enable_if_t<is_empty_placeholder<Action>::value, int> = 0>
auto create_action_if_empty(Action&&) {
    return [] {};
}

} // namespace detail

template <class Source, class Event, class Guard, class Action, class Destination>
constexpr auto make_transition(Source, Event, Guard&& guard, Action&& action, Destination) {
    auto updated_guard = detail::create_guard_if_empty(std::forward<Guard>(guard));
    using UpdatedGuard = decltype(updated_guard);

    auto updated_action = detail::create_action_if_empty(std::forward<Action>(action));
    using UpdatedAction = decltype(updated_action);

    using UpdatedDestination = std::
        conditional_t<detail::is_empty_placeholder<Destination>::value, State<void>, Destination>;

    return Transition<Source, Event, UpdatedGuard, UpdatedAction, UpdatedDestination>{
        std::forward<UpdatedGuard>(updated_guard), std::forward<UpdatedAction>(updated_action)};
}

} // namespace transition
} // namespace state_machine
