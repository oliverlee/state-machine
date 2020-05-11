#include "state_machine/traits.h"

#include "gtest/gtest.h"
#include <type_traits>
#include <utility>
#include <vector>

using namespace state_machine;

TEST(traits, is_specialization_of) {
    using MyPair = std::pair<int, int>;
    static_assert(aux::is_specialization_of<std::pair, MyPair>::value, "");
    static_assert(!aux::is_specialization_of<std::vector, MyPair>::value, "");
}

TEST(traits, is_copy_or_move_constructible) {
    struct A {};

    static_assert(std::is_copy_constructible<A>::value, "");
    static_assert(std::is_move_constructible<A>::value, "");
    static_assert(aux::is_copy_or_move_constructible<A>::value, "");

    struct B {
        B() = default;
        B(B&&) = default;
        B(const B&) = delete;
    };

    static_assert(!std::is_copy_constructible<B>::value, "");
    static_assert(std::is_move_constructible<B>::value, "");
    static_assert(aux::is_copy_or_move_constructible<B>::value, "");

    struct C {
        C() = default;
        C(C&&) = delete;
        C(const C&) = default;
    };

    static_assert(std::is_copy_constructible<C>::value, "");
    static_assert(!std::is_move_constructible<C>::value, "");
    static_assert(aux::is_copy_or_move_constructible<C>::value, "");

    struct D {
        D() = default;
        D(D&&) = delete;
        D(const D&) = delete;
    };

    static_assert(!std::is_copy_constructible<D>::value, "");
    static_assert(!std::is_move_constructible<D>::value, "");
    static_assert(!aux::is_copy_or_move_constructible<D>::value, "");
}
