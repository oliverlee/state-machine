#include "state_machine.h"
#include "state_machine/transition/transition.h"

#include "gtest/gtest.h"
#include <type_traits>

namespace {
using ::state_machine::event;
using ::state_machine::state;
using ::state_machine::placeholder::_;

using ::state_machine::transition::make_transition;

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

TEST(transition, invoke_guard_0) {
    auto transition = make_transition(state<s1>, event<e1>, _, _, _);

    EXPECT_EQ(transition.guard(), true);

    EXPECT_EQ(transition.invoke_guard(s1{}, e1{}), true);
}

TEST(transition, invoke_guard_s) {
    auto transition = make_transition(
        state<s1>, event<e1>, [](const s1&) { return true; }, _, _);

    EXPECT_EQ(transition.guard(s1{}), true);

    EXPECT_EQ(transition.invoke_guard(s1{}, e1{}), true);
}

TEST(transition, invoke_guard_e) {
    auto transition = make_transition(
        state<s1>, event<e1>, [](const e1&) { return true; }, _, _);

    EXPECT_EQ(transition.guard(e1{}), true);

    EXPECT_EQ(transition.invoke_guard(s1{}, e1{}), true);
}

TEST(transition, invoke_guard_se) {
    auto transition = make_transition(
        state<s1>, event<e1>, [](const s1&, const e1&) { return true; }, _, _);

    EXPECT_EQ(transition.guard(s1{}, e1{}), true);

    EXPECT_EQ(transition.invoke_guard(s1{}, e1{}), true);
}

TEST(transition, invoke_action_0) {
    auto transition = make_transition(
        state<s1>, event<e1>, _, [] { return s2{}; }, state<s2>);

    auto s = s1{};
    auto e = e1{};

    static_assert(std::is_same<decltype(transition.action()), s2>::value, "");
    static_assert(std::is_same<decltype(transition.invoke_action(s, e)), s2>::value, "");
}

TEST(transition, invoke_action_s) {
    auto transition = make_transition(
        state<s1>, event<e1>, _, [](s1&) { return s2{}; }, state<s2>);

    auto s = s1{};
    auto e = e1{};

    static_assert(std::is_same<decltype(transition.action(s)), s2>::value, "");
    static_assert(std::is_same<decltype(transition.invoke_action(s, e)), s2>::value, "");
}

TEST(transition, invoke_action_e) {
    auto transition = make_transition(
        state<s1>, event<e1>, _, [](e1&) { return s2{}; }, state<s2>);

    auto s = s1{};
    auto e = e1{};

    static_assert(std::is_same<decltype(transition.action(e)), s2>::value, "");
    static_assert(std::is_same<decltype(transition.invoke_action(s, e)), s2>::value, "");
}

TEST(transition, invoke_action_se) {
    auto transition = make_transition(
        state<s1>, event<e1>, _, [](s1&, e1&) { return s2{}; }, state<s2>);


    auto s = s1{};
    auto e = e1{};

    static_assert(std::is_same<decltype(transition.action(s, e)), s2>::value, "");
    static_assert(std::is_same<decltype(transition.invoke_action(s, e)), s2>::value, "");
}
