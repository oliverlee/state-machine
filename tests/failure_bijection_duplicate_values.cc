#include "state_machine/containers.h"

#include <type_traits>
#include <utility>

namespace {
using state_machine::containers::bijection;

using B = bijection<std::pair<std::integral_constant<int, 0>, std::integral_constant<int, 0>>,
                    std::pair<std::integral_constant<int, 1>, std::integral_constant<int, 0>>>;
} // namespace

int main() {
    static_assert(B::contains_key<std::integral_constant<int, 0>>::value, "");
    return 0;
}
