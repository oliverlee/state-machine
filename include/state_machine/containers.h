#pragma once

#include "state_machine/containers/basic.h"
#include "state_machine/containers/mapping.h"
#include "state_machine/containers/operations.h"

#include <type_traits>

namespace state_machine {
namespace containers {

using ::state_machine::containers::basic::identity;
using ::state_machine::containers::basic::inheritor;
using ::state_machine::containers::basic::list;
using ::state_machine::containers::mapping::surjection;

namespace op {

using ::state_machine::containers::operations::contains;
using ::state_machine::containers::operations::filter;
using ::state_machine::containers::operations::make_unique;
using ::state_machine::containers::operations::map;

} // namespace op
} // namespace containers
} // namespace state_machine
