#pragma once

#include "state_machine/backport.h"

#include <cstddef>
#include <type_traits>

namespace state_machine {
namespace aux {
// General purpose extensions to type_traits

// Check if a type is a template specialization
template <template <class...> class Template, class T>
struct is_specialization_of : std::false_type {};
template <template <class...> class Template, class... Args>
struct is_specialization_of<Template, Template<Args...>> : std::true_type {};

// Check if a type is copy or move constructible
template <class T>
using is_copy_or_move_constructible =
    stdx::disjunction<std::is_copy_constructible<T>, std::is_move_constructible<T>>;

// A convenience type alias for working an index_map.
template <std::size_t I>
using index_constant = std::integral_constant<std::size_t, I>;

} // namespace aux
} // namespace state_machine
