#include "state_machine/containers.h"

#include "gtest/gtest.h"
#include <type_traits>

namespace {
using state_machine::containers::bijection;
using state_machine::containers::identity;
using state_machine::containers::index_map;
using state_machine::containers::inheritor;
using state_machine::containers::list;
using state_machine::containers::surjection;
namespace op = state_machine::containers::op;
namespace aux = state_machine::aux;

struct A {};
struct B {};
} // namespace

TEST(containers_basic, identity) {
    static_assert(std::is_same<A, identity<A>::type>::value, "");
    static_assert(!std::is_same<B, identity<A>::type>::value, "");
}

TEST(containers_op, contains) {
    static_assert(op::contains<A, inheritor<A, B>>::value, "");
    static_assert(op::contains<A, inheritor<A>>::value, "");
    static_assert(!op::contains<A, inheritor<B>>::value, "");

    static_assert(op::contains<A, list<A, B>>::value, "");
    static_assert(op::contains<A, list<A>>::value, "");
    static_assert(!op::contains<A, list<B>>::value, "");
}

TEST(containers_op, make_unique) {
    static_assert(std::is_same<list<A, B>, op::make_unique<list<A, B>>>::value, "");
    static_assert(std::is_same<list<A, B>, op::make_unique<list<A, A, B, B>>>::value, "");

    static_assert(std::is_same<inheritor<A, B>, op::make_unique<inheritor<A, B>>>::value, "");
    static_assert(std::is_same<inheritor<A, B>, op::make_unique<inheritor<A, A, B, B>>>::value, "");

    static_assert(!std::is_same<list<A, B>, op::make_unique<inheritor<A, B>>>::value, "");
    static_assert(!std::is_same<list<A, B>, op::make_unique<inheritor<A, A, B, B>>>::value, "");
}

TEST(containers_op, filter) {
    static_assert(std::is_integral<bool>::value, "");
    static_assert(std::is_integral<int>::value, "");
    static_assert(!std::is_integral<float>::value, "");
    static_assert(!std::is_integral<double>::value, "");

    static_assert(std::is_same<list<>, op::filter<std::is_integral, list<>>>::value, "");
    static_assert(std::is_same<list<int>, op::filter<std::is_integral, list<int>>>::value, "");
    static_assert(std::is_same<list<>, op::filter<std::is_integral, list<double>>>::value, "");

    using Input = list<bool, int, float, double>;
    using Expected = list<bool, int>;
    static_assert(std::is_same<Expected, op::filter<std::is_integral, Input>>::value, "");
}

namespace {

template <class T>
struct get_first {
    using type = typename T::first_type;
};

} // namespace

TEST(containers_op, map) {
    using Input = list<std::pair<char, int>, std::pair<float, double>>;
    using Expected = list<char, float>;

    static_assert(std::is_same<Expected, op::map<get_first, Input>>::value, "");
}

namespace {

using S0 = surjection<std::pair<std::integral_constant<int, 0>, std::integral_constant<int, 0>>,
                      std::pair<std::integral_constant<int, 1>, std::integral_constant<int, 1>>,
                      std::pair<std::integral_constant<int, 2>, std::integral_constant<int, 2>>,
                      std::pair<std::integral_constant<int, 3>, std::integral_constant<int, 3>>,
                      std::pair<std::integral_constant<int, 4>, std::integral_constant<int, 4>>>;
using S1 = surjection<std::pair<std::integral_constant<int, 0>, std::integral_constant<int, 0>>,
                      std::pair<std::integral_constant<int, 1>, std::integral_constant<int, 1>>,
                      std::pair<std::integral_constant<int, 2>, std::integral_constant<int, 0>>,
                      std::pair<std::integral_constant<int, 3>, std::integral_constant<int, 1>>,
                      std::pair<std::integral_constant<int, 4>, std::integral_constant<int, 0>>>;
} // namespace

TEST(containers_surjection, at_key) {

    static_assert(std::is_same<S0::at_key<std::integral_constant<int, 0>>,
                               std::integral_constant<int, 0>>::value,
                  "");
    static_assert(std::is_same<S0::at_key<std::integral_constant<int, 1>>,
                               std::integral_constant<int, 1>>::value,
                  "");
    static_assert(std::is_same<S0::at_key<std::integral_constant<int, 2>>,
                               std::integral_constant<int, 2>>::value,
                  "");
    static_assert(std::is_same<S0::at_key<std::integral_constant<int, 3>>,
                               std::integral_constant<int, 3>>::value,
                  "");
    static_assert(std::is_same<S0::at_key<std::integral_constant<int, 4>>,
                               std::integral_constant<int, 4>>::value,
                  "");
    static_assert(std::is_same<S0::at_key<std::integral_constant<int, 5>>, void>::value, "");

    static_assert(std::is_same<S1::at_key<std::integral_constant<int, 0>>,
                               std::integral_constant<int, 0>>::value,
                  "");
    static_assert(std::is_same<S1::at_key<std::integral_constant<int, 1>>,
                               std::integral_constant<int, 1>>::value,
                  "");
    static_assert(std::is_same<S1::at_key<std::integral_constant<int, 2>>,
                               std::integral_constant<int, 0>>::value,
                  "");
    static_assert(std::is_same<S1::at_key<std::integral_constant<int, 3>>,
                               std::integral_constant<int, 1>>::value,
                  "");
    static_assert(std::is_same<S1::at_key<std::integral_constant<int, 4>>,
                               std::integral_constant<int, 0>>::value,
                  "");
    static_assert(std::is_same<S1::at_key<std::integral_constant<int, 5>>, void>::value, "");
}

TEST(containers_surjection, contains_key) {

    static_assert(S0::contains_key<std::integral_constant<int, 0>>::value, "");
    static_assert(S0::contains_key<std::integral_constant<int, 1>>::value, "");
    static_assert(S0::contains_key<std::integral_constant<int, 2>>::value, "");
    static_assert(S0::contains_key<std::integral_constant<int, 3>>::value, "");
    static_assert(S0::contains_key<std::integral_constant<int, 4>>::value, "");
    static_assert(!S0::contains_key<std::integral_constant<int, 5>>::value, "");

    static_assert(S1::contains_key<std::integral_constant<int, 0>>::value, "");
    static_assert(S1::contains_key<std::integral_constant<int, 1>>::value, "");
    static_assert(S1::contains_key<std::integral_constant<int, 2>>::value, "");
    static_assert(S1::contains_key<std::integral_constant<int, 3>>::value, "");
    static_assert(S1::contains_key<std::integral_constant<int, 4>>::value, "");
    static_assert(!S1::contains_key<std::integral_constant<int, 5>>::value, "");
}

namespace {

using B0 = bijection<std::pair<std::integral_constant<int, 0>, std::integral_constant<int, 0>>,
                     std::pair<std::integral_constant<int, 1>, std::integral_constant<int, 1>>,
                     std::pair<std::integral_constant<int, 2>, std::integral_constant<int, 2>>,
                     std::pair<std::integral_constant<int, 3>, std::integral_constant<int, 3>>,
                     std::pair<std::integral_constant<int, 4>, std::integral_constant<int, 4>>>;
} // namespace

TEST(containers_bijection, at_key) {

    static_assert(std::is_same<B0::at_key<std::integral_constant<int, 0>>,
                               std::integral_constant<int, 0>>::value,
                  "");
    static_assert(std::is_same<B0::at_key<std::integral_constant<int, 1>>,
                               std::integral_constant<int, 1>>::value,
                  "");
    static_assert(std::is_same<B0::at_key<std::integral_constant<int, 2>>,
                               std::integral_constant<int, 2>>::value,
                  "");
    static_assert(std::is_same<B0::at_key<std::integral_constant<int, 3>>,
                               std::integral_constant<int, 3>>::value,
                  "");
    static_assert(std::is_same<B0::at_key<std::integral_constant<int, 4>>,
                               std::integral_constant<int, 4>>::value,
                  "");
    static_assert(std::is_same<B0::at_key<std::integral_constant<int, 5>>, void>::value, "");
}

TEST(containers_bijection, contains_key) {

    static_assert(B0::contains_key<std::integral_constant<int, 0>>::value, "");
    static_assert(B0::contains_key<std::integral_constant<int, 1>>::value, "");
    static_assert(B0::contains_key<std::integral_constant<int, 2>>::value, "");
    static_assert(B0::contains_key<std::integral_constant<int, 3>>::value, "");
    static_assert(B0::contains_key<std::integral_constant<int, 4>>::value, "");
    static_assert(!B0::contains_key<std::integral_constant<int, 5>>::value, "");
}

TEST(containers_bijection, at_value) {

    static_assert(std::is_same<B0::at_value<std::integral_constant<int, 0>>,
                               std::integral_constant<int, 0>>::value,
                  "");
    static_assert(std::is_same<B0::at_value<std::integral_constant<int, 1>>,
                               std::integral_constant<int, 1>>::value,
                  "");
    static_assert(std::is_same<B0::at_value<std::integral_constant<int, 2>>,
                               std::integral_constant<int, 2>>::value,
                  "");
    static_assert(std::is_same<B0::at_value<std::integral_constant<int, 3>>,
                               std::integral_constant<int, 3>>::value,
                  "");
    static_assert(std::is_same<B0::at_value<std::integral_constant<int, 4>>,
                               std::integral_constant<int, 4>>::value,
                  "");
    static_assert(std::is_same<B0::at_value<std::integral_constant<int, 5>>, void>::value, "");
}

TEST(containers_bijection, contains_value) {

    static_assert(B0::contains_value<std::integral_constant<int, 0>>::value, "");
    static_assert(B0::contains_value<std::integral_constant<int, 1>>::value, "");
    static_assert(B0::contains_value<std::integral_constant<int, 2>>::value, "");
    static_assert(B0::contains_value<std::integral_constant<int, 3>>::value, "");
    static_assert(B0::contains_value<std::integral_constant<int, 4>>::value, "");
    static_assert(!B0::contains_value<std::integral_constant<int, 5>>::value, "");
}

TEST(containers_index_map, at_value) {

    using IM = index_map<bool, char, int, float, double>;

    static_assert(std::is_same<IM::at_value<aux::index_constant<0>>, bool>::value, "");
    static_assert(std::is_same<IM::at_value<aux::index_constant<1>>, char>::value, "");
    static_assert(std::is_same<IM::at_value<aux::index_constant<2>>, int>::value, "");
    static_assert(std::is_same<IM::at_value<aux::index_constant<3>>, float>::value, "");
    static_assert(std::is_same<IM::at_value<aux::index_constant<4>>, double>::value, "");
    static_assert(std::is_same<IM::at_value<aux::index_constant<5>>, void>::value, "");
}
