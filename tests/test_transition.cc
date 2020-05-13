#include "state_machine.h"

#include "gtest/gtest.h"
#include <type_traits>

namespace {
using ::state_machine::event;
using ::state_machine::make_transition;
using ::state_machine::state;
using ::state_machine::placeholder::_;

struct s1 {};
struct s2 {};

struct e1 {};
struct e2 {
    e2(int x) : value{x} {}
    int value;
};

} // namespace

TEST(transition, complete) {
    auto transition = make_transition(
        state<s1>,
        event<e1>,
        [](const auto& s, const auto& e) {
            (void)s;
            (void)e;
            return true;
        },
        [](auto& s, auto& e) {
            (void)s;
            (void)e;
            return s2{};
        },
        state<s2>);

    EXPECT_EQ(transition.guard(s1{}, e1{}), true);

    auto s = s1{};
    auto e = e1{};
    static_assert(std::is_same<decltype(transition.action(s, e)), s2>::value, "");
}

TEST(transition, empty_guard) {
    auto transition = make_transition(
        state<s1>,
        event<e1>,
        _,
        [](auto& s, auto& e) {
            (void)s;
            (void)e;
            return s2{};
        },
        state<s2>);

    EXPECT_EQ(transition.guard(), true);

    auto s = s1{};
    auto e = e1{};
    static_assert(std::is_same<decltype(transition.action(s, e)), s2>::value, "");
}

TEST(transition, empty_dest) {
    auto transition = make_transition(
        state<s1>, event<e1>, [] { return true; }, [] {}, _);

    EXPECT_EQ(transition.guard(), true);
    static_assert(std::is_same<decltype(transition.action()), void>::value, "");
}

TEST(transition, empty_action_dest) {
    auto transition = make_transition(
        state<s1>, event<e1>, [] { return true; }, _, _);

    EXPECT_EQ(transition.guard(), true);
    static_assert(std::is_same<decltype(transition.action()), void>::value, "");
}

TEST(transition, empty_guard_action_dest) {
    auto transition = make_transition(state<s1>, event<e1>, _, _, _);

    EXPECT_EQ(transition.guard(), true);
    static_assert(std::is_same<decltype(transition.action()), void>::value, "");
}

TEST(transition, transition_size_empty_base_optimization) {
    auto transition = make_transition(
        state<s1>,
        event<e1>,
        [](const auto& s, const auto& e) {
            (void)s;
            (void)e;
            return true;
        },
        [](auto& s, auto& e) {
            (void)s;
            (void)e;
            return s2{};
        },
        state<s2>);

    static_assert(sizeof(transition) == 1, "");
}

TEST(transition, transition_size_guard_capture_int) {
    int x = 42;

    auto transition = make_transition(
        state<s1>,
        event<e2>,
        [x](const auto& e) { return e.value > x; },
        []() { return s2{}; },
        state<s2>);

    static_assert(sizeof(transition) == sizeof(x), "");
}

TEST(transition, guard_moved_into_transition_size) {
    static constexpr int threshold = 42;

    auto transition = make_transition(
        state<s1>,
        event<e2>,
        [x = std::make_unique<int>(threshold)](const e2 e) { return e.value > *x; },
        _,
        _);

    EXPECT_FALSE(transition.guard(e2{threshold}));
    EXPECT_TRUE(transition.guard(e2{threshold + 1}));
}
