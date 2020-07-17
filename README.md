# state machine
A header-only library to generate small and efficient state machines.

## What's this about?
[Wikipedia](https://en.wikipedia.org/wiki/UML_state_machine) has a fairly good
description of state machines. This library lets you generate state machines at
compile time with some template magic.

Currently, functionality is a bit limited. Hierarchically nested states,
orthogonal regions, and event deferral are not yet implemented. But basic
concepts are there: states, events, extended states, guard conditions,
transition actions, run-to-completion, and entry/exit actions.

## How do I use this?
In general, you will need to define states, events, actions, and guards.
The library will take care of creating the transition table, state machine, and
handling all the different transitions between states -- all you have to do is
call `process_event`!

Note that in this library, an action is a
[Callable](https://en.cppreference.com/w/cpp/named_req/Callable) that returns
the destination state. The reasoning for this is to encapsulate extended state,
but allow transfer between states (e.g. moving a buffer from a 'read' state to a
'process' state).

Check out the [examples](./examples).

In order to build the examples, you'll need a compiler supporting C++14 and
[CMake](https://cmake.org/) or [Bazel](https://bazel.build/).

[Google Test](https://github.com/google/googletest) is required to run the
tests and is included as a submodule. Make sure to initialize it after cloning
this repo.

