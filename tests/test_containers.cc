#include "gtest/gtest.h"

#include "state_machine/containers.h"
#include <type_traits>

namespace {
using state_machine::containers::identity;
using state_machine::containers::inheritor;
using state_machine::containers::list;
namespace op = state_machine::containers::op;

struct A {};
struct B {};
} // namespace

TEST(containers, identity) {
    static_assert(std::is_same<A, identity<A>::type>::value, "");
    static_assert(!std::is_same<B, identity<A>::type>::value, "");
}

TEST(containers, contains) {
    static_assert(op::contains<A, inheritor<A, B>>::value, "");
    static_assert(op::contains<A, inheritor<A>>::value, "");
    static_assert(!op::contains<A, inheritor<B>>::value, "");

    static_assert(op::contains<A, list<A, B>>::value, "");
    static_assert(op::contains<A, list<A>>::value, "");
    static_assert(!op::contains<A, list<B>>::value, "");
}

TEST(containers, make_unique) {
    static_assert(std::is_same<list<A, B>, op::make_unique<list<A, B>>>::value,
                  "");
    static_assert(
        std::is_same<list<A, B>, op::make_unique<list<A, A, B, B>>>::value, "");

    static_assert(
        std::is_same<inheritor<A, B>, op::make_unique<inheritor<A, B>>>::value,
        "");
    static_assert(std::is_same<inheritor<A, B>,
                               op::make_unique<inheritor<A, A, B, B>>>::value,
                  "");

    static_assert(
        !std::is_same<list<A, B>, op::make_unique<inheritor<A, B>>>::value, "");
    static_assert(!std::is_same<list<A, B>,
                                op::make_unique<inheritor<A, A, B, B>>>::value,
                  "");
}

TEST(containers, filter) {
    static_assert(std::is_integral<bool>::value, "");
    static_assert(std::is_integral<int>::value, "");
    static_assert(!std::is_integral<float>::value, "");
    static_assert(!std::is_integral<double>::value, "");

    static_assert(
        std::is_same<list<>, op::filter<std::is_integral, list<>>>::value, "");
    static_assert(
        std::is_same<list<int>, op::filter<std::is_integral, list<int>>>::value,
        "");
    static_assert(
        std::is_same<list<>, op::filter<std::is_integral, list<double>>>::value,
        "");

    using Input = list<bool, int, float, double>;
    using Expected = list<bool, int>;
    static_assert(
        std::is_same<Expected, op::filter<std::is_integral, Input>>::value, "");
}
