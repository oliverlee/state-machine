load(":configure.bzl", "configure_local_variables")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

COMPDB_VERSION="0.4.4"

local_repository(
    name = "googletest",
    path = "extern/googletest",
)

configure_local_variables(
    name = "local_config",
    variable_template = "//:variables.bzl.tpl",
)

http_archive(
    name = "com_grail_compdb",
    strip_prefix = "bazel-compilation-database-%s" % COMPDB_VERSION,
    urls = [
        "https://github.com/grailbio/bazel-compilation-database/archive/%s.tar.gz" % COMPDB_VERSION
    ],
    sha256 = "70d9dbc7cf68b81e9548c96c75ecfa1d46dd8acab6f325aa4bc8efd6d4a88098",
)

new_local_repository(
    name = "local_tidy_config",
    path = "/usr/local/opt/llvm/bin/",
    build_file_content = """
sh_binary(
    name = "clang-tidy-bin",
    srcs = ["clang-tidy"],
    visibility = ["//visibility:public"],
)
""")
