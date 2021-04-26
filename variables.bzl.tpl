COMMON_CXX_WARN_OPTS = [
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wnon-virtual-dtor",
    "-Wold-style-cast",
    "-Wcast-align",
    "-Wunused",
    "-Woverloaded-virtual",
    "-Wpedantic",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wformat=2",
]

CLANG_CXX_WARN_OPTS = [
    "-Wnull-dereference",
]

GCC_CXX_WARN_OPTS = [
    "-Wshadow=compatible-local",
    "-Wlogical-op",
    "-Wuseless-cast",
    "-Wduplicated-cond",
    "-Wduplicated-branches",
    "-Wmisleading-indentation",
]

STATE_MACHINE_INCLUDE_DIR_OPT = ["-Iinclude"]
STATE_MACHINE_DEFAULT_COPTS = STATE_MACHINE_INCLUDE_DIR_OPT + COMMON_CXX_WARN_OPTS + %{compiler_name}_CXX_WARN_OPTS
