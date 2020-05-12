#pragma once

#include <type_traits>

namespace state_machine {
namespace containers {
namespace basic {
// Type containers

template <class T>
struct identity {
    using type = T;
};

template <class...>
struct list {
    using type = list;
};

template <class... Ts>
struct inheritor : Ts... {
    using type = inheritor;
};

} // namespace basic
} // namespace containers
} // namespace state_machine
