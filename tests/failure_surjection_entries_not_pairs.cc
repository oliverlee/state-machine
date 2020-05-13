#include "state_machine/containers.h"

#include <tuple>
#include <type_traits>
#include <utility>

namespace {
using state_machine::containers::surjection;

using S = surjection<std::tuple<std::integral_constant<int, 0>, std::integral_constant<int, 0>>,
                     std::tuple<std::integral_constant<int, 1>, std::integral_constant<int, 1>>>;
} // namespace

int main() {
    static_assert(S::contains_key<std::integral_constant<int, 0>>::value, "");
    return 0;
}
