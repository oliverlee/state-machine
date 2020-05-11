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
