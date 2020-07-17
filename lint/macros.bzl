"""Defines macros to perform lint actions.
"""


def lint_sources(name, sources, compdb, dependencies, options=None, visibility=None):
    """Runs linting on given source files.

    This macro defines a test target that depends on intermediate lint rules. An
    intermediate lint rule is created for every item in `files`.

    Args:
        name: A unique name for this rule.
        sources: A sequence of source files to lint. Each item in the sequence
                 generates an independent and intermediate lint rule.
        compdb: A compilation database specifying compiler options for items in
                `files`.
        dependencies: A sequence of dependencies for this rule.
        options: A string of additional options to forward.
        visibility: The visibility of this rule.
    """
    tidy_bin = "@local_tidy_config//:clang-tidy-bin"
    tidy_base_config = "//:.clang-tidy"
    tidy_extra_opts = " " + options if options else ""
    outputs = []

    for source in sources:
        rulename = "_lint_" + source
        out = rulename + ".out"
        outputs.append(out)

        native.genrule(
            name = rulename,
            srcs = [compdb, source, tidy_base_config] + dependencies,
            outs = [out],
            tools = [tidy_bin],
            cmd = (
                "$(location %s)" % tidy_bin
                + " -p $(location %s)" % compdb
                + tidy_extra_opts
                + " $(location %s)" % source
                + " && md5sum $$(echo $(SRCS) | sort) > $@"
            ),
            testonly = True,
        )

    native.sh_test(
        name = name,
        srcs = ["//lint:dummy.sh"],
        data = outputs,
        visibility = visibility,
    )
