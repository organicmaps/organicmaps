import os
import subprocess

from maps_generator.generator import settings
from maps_generator.generator.exceptions import BadExitStatusError
from maps_generator.generator.exceptions import wait_and_raise_if_fail


def build_osmtools(path, output=subprocess.DEVNULL, error=subprocess.DEVNULL):
    src = {
        settings.OSM_TOOL_UPDATE: "osmupdate.c",
        settings.OSM_TOOL_FILTER: "osmfilter.c",
        settings.OSM_TOOL_CONVERT: "osmconvert.c",
    }
    ld_flags = ("-lz",)
    cc = []
    result = {}
    for executable, src in src.items():
        out = os.path.join(settings.OSM_TOOLS_PATH, executable)
        op = [
            settings.OSM_TOOLS_CC,
            *settings.OSM_TOOLS_CC_FLAGS,
            "-o",
            out,
            os.path.join(path, src),
            *ld_flags,
        ]
        s = subprocess.Popen(op, stdout=output, stderr=error)
        cc.append(s)
        result[executable] = out

    messages = []
    for c in cc:
        if c.wait() != os.EX_OK:
            messages.append(f"The launch of {' '.join(c.args)} failed.")
    if messages:
        raise BadExitStatusError("\n".split(messages))

    return result


def osmconvert(
    name_executable,
    in_file,
    out_file,
    output=subprocess.DEVNULL,
    error=subprocess.DEVNULL,
    run_async=False,
    **kwargs,
):
    env = os.environ.copy()
    env["PATH"] = f"{settings.OSM_TOOLS_PATH}:{env['PATH']}"
    p = subprocess.Popen(
        [
            name_executable,
            in_file,
            "--drop-author",
            "--drop-version",
            "--out-o5m",
            f"-o={out_file}",
        ],
        env=env,
        stdout=output,
        stderr=error,
    )
    if run_async:
        return p
    else:
        wait_and_raise_if_fail(p)


def osmupdate(
    name_executable,
    in_file,
    out_file,
    output=subprocess.DEVNULL,
    error=subprocess.DEVNULL,
    run_async=False,
    **kwargs,
):
    env = os.environ.copy()
    env["PATH"] = f"{settings.OSM_TOOLS_PATH}:{env['PATH']}"
    p = subprocess.Popen(
        [
            name_executable,
            "--drop-author",
            "--drop-version",
            "--out-o5m",
            "-v",
            in_file,
            out_file,
        ],
        env=env,
        stdout=output,
        stderr=error,
    )
    if run_async:
        return p
    else:
        wait_and_raise_if_fail(p)


def osmfilter(
    name_executable,
    in_file,
    out_file,
    output=subprocess.DEVNULL,
    error=subprocess.DEVNULL,
    run_async=False,
    **kwargs,
):
    env = os.environ.copy()
    env["PATH"] = f"{settings.OSM_TOOLS_PATH}:{env['PATH']}"
    args = [name_executable, in_file, f"-o={out_file}"] + [
        f"--{k.replace('_', '-')}={v}" for k, v in kwargs.items()
    ]
    p = subprocess.Popen(args, env=env, stdout=output, stderr=error)
    if run_async:
        return p
    else:
        wait_and_raise_if_fail(p)
