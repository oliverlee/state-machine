#pragma once

#include "state_machine/transition/transition_table.h"

#include <tuple>

namespace state_machine {
namespace state_machine {

template <class Table>
class StateMachine {
    static_assert(transition::is_table<Table>::value,
                  "A `StateMachine` must be created from a `Table`");

  public:
    using type = StateMachine<Table>;
    using state_types = typename Table::state_types;
    using event_types = typename Table::event_types;
    using initial_state_type =
        typename std::tuple_element_t<0, typename Table::data_type>::source_type;

    StateMachine(Table&& table) : table_{std::forward<Table>(table)} {}

  private:
    Table table_;
};

} // namespace state_machine
} // namespace state_machine
