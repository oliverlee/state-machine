#pragma once

#include "state_machine/transition.h"

namespace state_machine {

using transition::make_transition;

namespace placeholder {
constexpr auto _ = transition::empty_placeholder;
} // namespace placeholder

template <class T>
constexpr auto state = transition::State<T>{};

template <class E>
constexpr auto event = transition::Event<E>{};

} // namespace state_machine
