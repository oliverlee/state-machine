#pragma once

#include "state_machine/state_machine.h"
#include "state_machine/transition/transition.h"

namespace state_machine {

using ::state_machine::state_machine::make_state_machine;
using ::state_machine::state_machine::process_status;
using ::state_machine::state_machine::StateMachine;

namespace placeholder {
constexpr auto _ = transition::empty_placeholder;
} // namespace placeholder

template <class T>
// NOLINTNEXTLINE(misc-definitions-in-headers)
constexpr auto state = transition::State<T>{};

template <class E>
// NOLINTNEXTLINE(misc-definitions-in-headers)
constexpr auto event = transition::Event<E>{};

template <class... Ts>
constexpr auto make_transition_table(Ts&&... args) noexcept {
    return transition::make_table_from_transition_args<Ts...>(std::forward<Ts>(args)...);
}

} // namespace state_machine
