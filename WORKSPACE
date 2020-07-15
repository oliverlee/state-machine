local_repository(
    name = "googletest",
    path = "extern/googletest",
)

load(":configure_copts.bzl", "configure_compiler_copts")
configure_compiler_copts(
    name = "generated_compiler_config",
    variable_template = "//:variables.bzl.tpl",
)
