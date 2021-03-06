#include "state_machine.h"
#include "state_machine/transition/transition_table.h"

#include "gtest/gtest.h"
#include <tuple>

namespace {
using ::state_machine::event;
using ::state_machine::state;
using ::state_machine::placeholder::_;

using ::state_machine::transition::make_row;
using ::state_machine::transition::make_table;
using ::state_machine::transition::make_table_from_transition_args;
using ::state_machine::transition::make_transition;

struct s1 {};
struct s2 {};
struct s3 {
    explicit s3(int x) : value{x} {}
    int value;
};

struct e1 {};
struct e2 {};
struct e3 {
    explicit e3(int x) : value{x} {}
    int value;
};

} // namespace

TEST(transition_table, make_table) {
    auto tb = make_table(make_row(make_transition(
                             state<s1>, event<e2>, _, [] { return s2{}; }, state<s2>)),
                         make_row(make_transition(
                             state<s1>, event<e1>, _, [] {}, _)));

    static_assert(decltype(tb)::size == 2, "");
}

TEST(transition_table, update_new_key) {
    auto tb1 = make_table(make_row(make_transition(
        state<s1>, event<e2>, _, [] { return s2{}; }, state<s2>)));

    auto tb2 = std::move(tb1).update(make_transition(
        state<s1>, event<e1>, _, [] {}, _));

    static_assert(decltype(tb2)::size == 2, "");
}

TEST(transition_table, update_existing_key) {
    auto tb1 = make_table(make_row(make_transition(
        state<s1>, event<e3>, [](auto e) { return e.value > 0; }, [] { return s2{}; }, state<s2>)));

    auto tb2 = std::move(tb1).update(make_transition(
        state<s1>, event<e3>, [](auto e) { return e.value <= 0; }, _, _));

    static_assert(decltype(tb2)::size == 1, "");

    const auto& row1 = std::get<0>(tb2.data());
    {
        const auto& transition = std::get<0>(row1.data());
        EXPECT_TRUE(transition.guard(e3{1}));
        EXPECT_FALSE(transition.guard(e3{0}));
    }
    {
        const auto& transition = std::get<1>(row1.data());
        EXPECT_FALSE(transition.guard(e3{1}));
        EXPECT_TRUE(transition.guard(e3{0}));
    }
}

TEST(transition_table, multiple_updates) {
    auto tb1 = make_table(make_row(make_transition(
        state<s1>, event<e2>, _, [] { return s2{}; }, state<s2>)));
    static_assert(decltype(tb1)::size == 1, "");

    auto tb2 = std::move(tb1).update(make_transition(
        state<s1>, event<e1>, _, [] {}, _));
    static_assert(decltype(tb2)::size == 2, "");

    auto tb3 = std::move(tb2).update(make_transition(
        state<s1>, event<e2>, _, [] { return s3{3}; }, state<s3>));
    static_assert(decltype(tb3)::size == 2, "");

    auto tb4 = std::move(tb3).update(make_transition(
        state<s1>, event<e3>, _, [](auto e) noexcept { return s3{e.value}; }, state<s3>));
    static_assert(decltype(tb4)::size == 3, "");

    auto tb5 = std::move(tb4).update(make_transition(
        state<s3>, event<e3>, [](auto e) { return e.value > 0; }, [] { return s2{}; }, state<s2>));
    static_assert(decltype(tb5)::size == 4, "");

    auto tb6 = std::move(tb5).update(make_transition(
        state<s3>,
        event<e3>,
        [](auto e) { return e.value == 0; },
        [] { return s3{0}; },
        state<s3>));
    static_assert(decltype(tb6)::size == 4, "");

    namespace tr = ::state_machine::transition;
    using FirstKey = std::decay_t<decltype(std::get<0>(tb6.data()))>::key_type;
    static_assert(std::is_same<FirstKey, tr::Key<tr::State<s1>, tr::Event<e2>>>::value, "");
}

TEST(transition_table, make_table_from_transition_args) {
    // clang-format off
    auto table = make_table_from_transition_args(
        state<s1>, event<e2>,                                   _,                         [] { return s2{}; }, state<s2>,
        state<s1>, event<e1>,                                   _,                                       [] {},         _,
        state<s1>, event<e2>,                                   _,                        [] { return s3{3}; }, state<s3>,
        state<s1>, event<e3>,                                   _, [](auto e) noexcept { return s3{e.value}; }, state<s3>,
        state<s3>, event<e3>,  [](auto e) { return e.value > 0; },                         [] { return s2{}; }, state<s2>,
        state<s3>, event<e3>, [](auto e) { return e.value == 0; },                        [] { return s3{0}; }, state<s3>);
    static_assert(decltype(table)::size == 4, "");
    // clang-format on

    namespace tr = ::state_machine::transition;
    using FirstKey = std::decay_t<decltype(std::get<0>(table.data()))>::key_type;
    static_assert(std::is_same<FirstKey, tr::Key<tr::State<s1>, tr::Event<e2>>>::value, "");
}
