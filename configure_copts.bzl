def _is_clang(repository_ctx, cc):
    return "clang" in repository_ctx.execute([cc, "-v"]).stderr

def _impl(repository_ctx):
    """Substitute %{compiler_name} with GCC or CLANG."""
    # Define an empty BUILD file to allow an external project
    repository_ctx.file("BUILD", "");

    cc = repository_ctx.os.environ.get("CC", "CC")

    compiler_name = "CLANG" if _is_clang(repository_ctx, cc) else "GCC"
    repository_ctx.template(
        "variables.bzl",
        repository_ctx.attr.variable_template,
        {"%{compiler_name}": compiler_name}
    )

configure_compiler_copts = repository_rule(
    implementation=_impl,
    attrs = {
        "variable_template": attr.label(
            mandatory = True,
            allow_single_file=True,
        ),
    },
    local=True,
    environ = ["CC"]
)
