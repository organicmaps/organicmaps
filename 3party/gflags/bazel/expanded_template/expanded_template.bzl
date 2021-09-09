def _impl(ctx):
    args = ctx.actions.args()
    args.add("--template", ctx.file.template)
    args.add("--output", ctx.outputs.out)
    args.add_all([k + ';' + v for k, v in ctx.attr.substitutions.items()])
    ctx.actions.run(
        executable = ctx.executable._bin,
        arguments = [args],
        inputs = [ctx.file.template],
        outputs = [ctx.outputs.out],
    )
    return [
        DefaultInfo(
            files = depset(direct = [ctx.outputs.out]),
            runfiles = ctx.runfiles(files = [ctx.outputs.out]),
        ),
    ]

expanded_template = rule(
    implementation = _impl,
    attrs = {
        "out": attr.output(mandatory = True),
        "template": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
        "substitutions": attr.string_dict(),
        "_bin": attr.label(
            default = "//bazel/expanded_template:expand_template",
            executable = True,
            allow_single_file = True,
            cfg = "host",
        ),
    },
)
