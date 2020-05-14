#include "state_machine.h"

#include "gtest/gtest.h"
#include <type_traits>

namespace {
using ::state_machine::event;
using ::state_machine::make_transition;
using ::state_machine::state;
using ::state_machine::placeholder::_;

using ::state_machine::transition::make_row;
using ::state_machine::transition::update_row;

struct s1 {};
struct s2 {};
struct s3 {};

struct e1 {
    e1(int x) : value{x} {}
    int value;
};

const auto t1 = []() {
    return make_transition(
        state<s1>,
        event<e1>,
        [](const auto& e) { return e.value > 0; },
        [] { return s2{}; },
        state<s2>);
};

const auto t2 = []() {
    return make_transition(
        state<s1>,
        event<e1>,
        [](const auto& e) { return e.value <= 0; },
        [] { return s3{}; },
        state<s3>);
};

} // namespace

TEST(transition_row, make_row_t1) {
    const auto row = make_row(t1());
    static_assert(decltype(row)::size == 1, "");

    EXPECT_TRUE(std::get<0>(row).guard(e1{1}));
    EXPECT_FALSE(std::get<0>(row).guard(e1{0}));
}

TEST(transition_row, make_row_t1t2) {
    const auto row = make_row(t1(), t2());
    static_assert(decltype(row)::size == 2, "");

    EXPECT_TRUE(std::get<0>(row).guard(e1{1}));
    EXPECT_FALSE(std::get<0>(row).guard(e1{0}));

    EXPECT_FALSE(std::get<1>(row).guard(e1{1}));
    EXPECT_TRUE(std::get<1>(row).guard(e1{0}));
}

TEST(transition_row, update_row) {
    auto row1 = make_row(t1());
    static_assert(decltype(row1)::size == 1, "");

    EXPECT_TRUE(std::get<0>(row1).guard(e1{1}));
    EXPECT_FALSE(std::get<0>(row1).guard(e1{0}));

    auto row2 = update_row(std::move(row1), t2());
    static_assert(decltype(row2)::size == 2, "");

    // update_row inserts the new transition at the front
    EXPECT_TRUE(std::get<1>(row2).guard(e1{1}));
    EXPECT_FALSE(std::get<1>(row2).guard(e1{0}));

    EXPECT_FALSE(std::get<0>(row2).guard(e1{1}));
    EXPECT_TRUE(std::get<0>(row2).guard(e1{0}));
}

TEST(transition_row, row_key_type) {
    namespace tr = ::state_machine::transition;

    const auto row = make_row(t1());
    static_assert(decltype(row)::size == 1, "");
    static_assert(
        std::is_same<decltype(row)::key_type, tr::Key<tr::State<s1>, tr::Event<e1>>>::value, "");
}
