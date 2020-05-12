#include "state_machine/backport.h"

#include "gtest/gtest.h"
#include <type_traits>

using namespace state_machine;

TEST(backport, negation) {
    static_assert(stdx::negation<std::is_same<int, float>>::value, "");
}

TEST(backport, conjunction) {
    static_assert(stdx::conjunction<std::is_same<int, int>,
                                    std::is_same<float, float>>::value,
                  "");

    static_assert(!stdx::conjunction<std::is_same<int, int>,
                                     std::is_same<int, float>>::value,
                  "");
}

TEST(backport, disjunction) {
    static_assert(stdx::disjunction<std::is_same<int, int>,
                                    std::is_same<int, float>>::value,
                  "");

    static_assert(!stdx::disjunction<std::is_same<float, double>,
                                     std::is_same<int, float>>::value,
                  "");
}
