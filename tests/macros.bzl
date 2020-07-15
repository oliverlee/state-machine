load("@generated_compiler_config//:variables.bzl", "STATE_MACHINE_DEFAULT_COPTS")

def add_unit_test(name, **kwargs):
    """Add a unit test with default options.

    This macro assumes the unit test requires a single source file which is
    derived from the `name` argument.
    """
    native.cc_test(
        name = name,
        size = "small",
        srcs = [name + ".cc"],
        copts = STATE_MACHINE_DEFAULT_COPTS,
        deps = [
            "@googletest//:gtest_main",
            "//:state_machine",
        ],
        **kwargs
    )
