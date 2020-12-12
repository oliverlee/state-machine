load(
    ":variables.bzl",
    "BREW_LLVM_HEAD_VERSION",
    "BREW_GCC_HEAD_VERSION",
)
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
    "variable_with_value",
)

def clang_tool_paths(bin_dir):
    return [
        tool_path(
            name = "gcc",
            path = bin_dir + "clang",
        ),
        tool_path(
            name = "ld",
            path = bin_dir + "lld",
        ),
        tool_path(
            name = "ar",
            path = bin_dir + "llvm-ar",
        ),
        tool_path(
            name = "cpp",
            path = "/bin/false",
        ),
        tool_path(
            name = "gcov",
            path = bin_dir + "llvm-cov",
        ),
        tool_path(
            name = "nm",
            path = bin_dir + "llvm-nm",
        ),
        tool_path(
            name = "objdump",
            path = bin_dir + "llvm-objdump",
        ),
        tool_path(
            name = "strip",
            path = bin_dir + "llvm-strip",
        ),
    ]

def gcc_tool_paths(bin_dir, version):
    return [
        tool_path(
            name = "gcc",
            path = bin_dir + "g++-" + version.major,
        ),
        tool_path(
            name = "ld",
            path = "/usr/bin/ld",
        ),
        tool_path(
            name = "ar",
            path = "/usr/bin/ar",
        ),
        tool_path(
            name = "cpp",
            path = bin_dir + "cpp-" + version.major,
        ),
        tool_path(
            name = "gcov",
            path = bin_dir + "gcov-" + version.major,
        ),
        tool_path(
            name = "nm",
            path = bin_dir + "gcc-nm-" + version.major,
        ),
        tool_path(
            name = "objdump",
            path = "/bin/false",
        ),
        tool_path(
            name = "strip",
            path = "/bin/false",
        ),
    ]

def _create_version(brew_version):
    return struct(
        brew = brew_version,
        full = brew_version.split("_")[0],
        major = brew_version.split(".")[0],
    )

def _is_head_version(compiler, version):
    if compiler == "clang":
        head_version = BREW_LLVM_HEAD_VERSION
    else:
        head_version = BREW_GCC_HEAD_VERSION

    return head_version == version.major

def brew_toolchain_dir(compiler, version):
    toolchain_root = "/usr/local/Cellar/" + compiler
    if not _is_head_version(compiler, version):
        toolchain_root += "@" + version.major
    return "%s/%s/" % (toolchain_root, version.brew)

def clang_paths(brew_version):
    version = _create_version(brew_version)
    tool_root = brew_toolchain_dir("llvm", version)

    usr_include_dir = "/Library/Developer/CommandLineTools/SDKs/"
    if BREW_LLVM_HEAD_VERSION != version.major:
        usr_include_dir += "MacOSX.sdk"
    else:
        usr_include_dir += "MacOSX10.15.sdk"
    usr_include_dir += "/usr/include"

    tool_include_dir = [
        tool_root + "include",
        tool_root + "lib/clang/%s/include" % (version.full),
    ]

    return clang_tool_paths(tool_root + "bin/"), tool_include_dir + [usr_include_dir]

def gcc_paths(brew_version):
    version = _create_version(brew_version)
    tool_root = brew_toolchain_dir("gcc", version)

    usr_include_dir = "/Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/usr/include"

    tool_include_dir = [
        tool_root + "include",
        tool_root + "lib/gcc/%s/gcc/x86_64-apple-darwin19/%s/include" % (version.major, version.full),
        tool_root + "lib/gcc/%s/gcc/x86_64-apple-darwin19/%s/include-fixed" % (version.major, version.full),
        tool_root + "lib/gcc/%s/gcc/x86_64-apple-darwin19.4.0/%s/include" % (version.major, version.full),
        tool_root + "lib/gcc/%s/gcc/x86_64-apple-darwin19.4.0/%s/include-fixed" % (version.major, version.full),
    ]

    return gcc_tool_paths(tool_root + "bin/", version), tool_include_dir + [usr_include_dir]

def _impl(ctx):
    brew_version = ctx.attr.version

    features = [
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [
                        ACTION_NAMES.cpp_link_executable,
                        ACTION_NAMES.cpp_link_dynamic_library,
                        ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                    ],
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-lstdc++",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]

    if ctx.attr.compiler == "clang":
        tool_paths, include_dir = clang_paths(brew_version)
    elif ctx.attr.compiler == "gcc":
        tool_paths, include_dir = gcc_paths(brew_version)

        features += [
            feature(
                name = "archiver_flags",
                enabled = True,
                flag_sets = [
                    flag_set(
                        actions = [
                            ACTION_NAMES.cpp_link_static_library,
                        ],
                        flag_groups = [
                            flag_group(flags = ["rcs"]),
                            flag_group(
                                flags = ["%{output_execpath}"],
                                expand_if_available = "output_execpath",
                            ),
                        ],
                    ),
                    flag_set(
                        actions = [ACTION_NAMES.cpp_link_static_library],
                        flag_groups = [
                            flag_group(
                                iterate_over = "libraries_to_link",
                                flag_groups = [
                                    flag_group(
                                        flags = ["%{libraries_to_link.name}"],
                                        expand_if_equal = variable_with_value(
                                            name = "libraries_to_link.type",
                                            value = "object_file",
                                        ),
                                    ),
                                    flag_group(
                                        flags = ["%{libraries_to_link.object_files}"],
                                        iterate_over = "libraries_to_link.object_files",
                                        expand_if_equal = variable_with_value(
                                            name = "libraries_to_link.type",
                                            value = "object_file_group",
                                        ),
                                    ),
                                ],
                                expand_if_available = "libraries_to_link",
                            ),
                        ],
                    ),
                ],
            ),
        ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = include_dir,
        toolchain_identifier = "local",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "unknown",
        compiler = ctx.attr.compiler,
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "compiler": attr.string(mandatory=True),
        "version": attr.string(mandatory=True),
    },
    provides = [CcToolchainConfigInfo],
)

def brew_cc_toolchain(name, compiler, version, visibility=None):
    """Represents a C++ toolchain installed by brew.

    Args:
        name: A unique name for this rule.
        compiler: Either `clang` or `gcc`.
        version: The compiler version.
        visibility: The visibility of this rule.
    """
    config_name = name + "_config"
    cc_toolchain_config(
        name = config_name,
        compiler = compiler,
        version = version,
    )

    native.cc_toolchain(
        name = name,
        toolchain_config = ":" + config_name,
        all_files = ":empty",
        compiler_files = ":empty",
        dwp_files = ":empty",
        linker_files = ":empty",
        objcopy_files = ":empty",
        strip_files = ":empty",
        supports_param_files = 0,
        visibility = visibility,
    )
