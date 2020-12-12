load(":cc_toolchain_config.bzl", "brew_cc_toolchain")
load(
    ":variables.bzl",
    "BREW_LLVM_10_VERSION",
    "BREW_LLVM_9_VERSION",
    "BREW_LLVM_8_VERSION",
    "BREW_GCC_10_VERSION",
    "BREW_GCC_7_VERSION",
    "BREW_GCC_5_VERSION",
)

package(default_visibility = ["//visibility:public"])

filegroup(name = "empty", visibility = ["//visibility:private"])

cc_toolchain_suite(
    name = "brew_toolchain_suite",
    toolchains = {
        "darwin|clang10": ":darwin_brew_clang_10_toolchain",
        "darwin|clang9": ":darwin_brew_clang_9_toolchain",
        "darwin|clang8": ":darwin_brew_clang_8_toolchain",
        "darwin|gcc10": ":darwin_brew_gcc_10_toolchain",
        "darwin|gcc7": ":darwin_brew_gcc_7_toolchain",
        "darwin|gcc5": ":darwin_brew_gcc_5_toolchain",
    }
)

brew_cc_toolchain("darwin_brew_clang_10_toolchain", "clang", BREW_LLVM_10_VERSION)
brew_cc_toolchain("darwin_brew_clang_9_toolchain", "clang", BREW_LLVM_9_VERSION)
brew_cc_toolchain("darwin_brew_clang_8_toolchain", "clang", BREW_LLVM_8_VERSION)
brew_cc_toolchain("darwin_brew_gcc_10_toolchain", "gcc", BREW_GCC_10_VERSION)
brew_cc_toolchain("darwin_brew_gcc_7_toolchain", "gcc", BREW_GCC_7_VERSION)
brew_cc_toolchain("darwin_brew_gcc_5_toolchain", "gcc", BREW_GCC_5_VERSION)
