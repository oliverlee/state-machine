#include "state_machine.h"

#include <cassert>
#include <iostream>

namespace {
namespace sm = state_machine;


// Simple Example.
// Based on an example from boost::msm.
// https://www.boost.org/doc/libs/1_73_0/libs/msm/doc/HTML/examples/SimpleTutorial.cpp

enum class DiskType { CD, DVD };

// states
struct Stopped {
    void on_entry() { std::cout << "entering: Stopped" << std::endl; }
    void on_exit() { std::cout << "leaving: Stopped" << std::endl; }
};
struct Open {
    void on_entry() { std::cout << "entering: Open" << std::endl; }
    void on_exit() { std::cout << "leaving: Open" << std::endl; }
};
struct Empty {
    void on_entry() { std::cout << "entering: Empty" << std::endl; }
    void on_exit() { std::cout << "leaving: Empty" << std::endl; }
};
struct Playing {
    void on_entry() { std::cout << "entering: Playing" << std::endl; }
    void on_exit() { std::cout << "leaving: Playing" << std::endl; }
};
struct Paused {};

// events
struct play {};
struct open_close {};
struct stop {};
struct pause {};
struct end_pause {};
struct cd_detected {
    constexpr cd_detected(DiskType diskType) : disc_type(diskType) {}

    DiskType disc_type;
};

// actions
// The transition table requires actions to create the destination state as this is how we can
// transfer "extended/internal" state between actual states (e.g. a large buffer of data from a
// `receive` state to a `process` state).
struct start_playback {
    constexpr auto operator()() const noexcept { return Playing{}; }
};
struct open_drawer {
    constexpr auto operator()() const noexcept { return Open{}; }
};
struct close_drawer {
    constexpr auto operator()() const noexcept { return Empty{}; }
};
struct stop_playback {
    constexpr auto operator()() const noexcept { return Stopped{}; }
};
struct pause_playback {
    constexpr auto operator()() const noexcept { return Paused{}; }
};
struct stop_and_open {
    constexpr auto operator()() const noexcept { return Open{}; }
};
struct resume_playback {
    constexpr auto operator()() const noexcept { return Playing{}; }
};

struct cd_detected_guard {
    constexpr auto operator()(const cd_detected& e) const noexcept -> bool {
        return e.disc_type == DiskType::CD;
    }
};

namespace player {

using ::state_machine::event;
using ::state_machine::state;
using ::state_machine::placeholder::_;

constexpr auto transition_table =
    // clang-format off
    sm::make_transition_table(
        state<Empty>, event<open_close>, _, open_drawer{}, state<Open>,
        state<Empty>, event<cd_detected>, cd_detected_guard{}, stop_playback{}, state<Stopped>,

        state<Stopped>, event<play>, _, start_playback{}, state<Playing>,
        state<Stopped>, event<open_close>, _, open_drawer{}, state<Open>,
        state<Stopped>, event<stop>, _, stop_playback{}, state<Stopped>,

        state<Open>, event<open_close>, _, close_drawer{}, state<Empty>,

        state<Playing>, event<stop>, _, stop_playback{}, state<Stopped>,
        state<Playing>, event<pause>, _, pause_playback{}, state<Paused>,
        state<Playing>, event<open_close>, _, stop_and_open{}, state<Open>,

        state<Paused>, event<end_pause>, _, resume_playback{}, state<Playing>,
        state<Paused>, event<stop>, _, stop_playback{}, state<Stopped>,
        state<Paused>, event<open_close>, _, stop_and_open{}, state<Open>
    );
// clang-format on

using Table = decltype(transition_table);
using StateMachine = sm::StateMachine<const Table&>;

} // namespace player

struct Player : player::StateMachine {
    Player() : player::StateMachine{player::transition_table} {}
};

} // namespace

int main() {
    // The state machine starts as soon as it is created.
    auto p = Player{};
    assert(p.is_state<Empty>());

    std::cout << '\n';

    p.process_event(open_close());
    assert(p.is_state<Open>());

    std::cout << '\n';

    p.process_event(open_close());
    assert(p.is_state<Empty>());

    std::cout << '\n';

    // will be rejected, wrong disk type
    p.process_event(cd_detected(DiskType::DVD));
    assert(p.is_state<Empty>());

    std::cout << '\n';

    p.process_event(cd_detected(DiskType::CD));
    assert(p.is_state<Stopped>());

    std::cout << '\n';

    p.process_event(play());
    assert(p.is_state<Playing>());

    std::cout << '\n';

    p.process_event(pause());
    assert(p.is_state<Paused>());

    std::cout << '\n';

    p.process_event(end_pause());
    assert(p.is_state<Playing>());

    std::cout << '\n';

    p.process_event(pause());
    assert(p.is_state<Paused>());

    std::cout << '\n';

    p.process_event(stop());
    assert(p.is_state<Stopped>());

    std::cout << '\n';

    // event leading to the same state
    // no action method called as it is not present in the transition table
    p.process_event(stop());
    assert(p.is_state<Stopped>());

    std::cout << '\n';

    return 0;
}
