"""Exercise optional Wayland dependencies and the bundled GLFW/ImGui sources.

The optional package-removal matrix runs only in a disposable Debian/Ubuntu
container. It leaves the Wayland development package removed.
"""

import argparse
from contextlib import contextmanager
import os
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--remove-packages", action="store_true")
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    for name in ("DISPLAY", "WAYLAND_DISPLAY", "XDG_SESSION_TYPE"):
        env.pop(name, None)

    def run(name, command, expected_success=True):
        result = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=300)
        (output / f"{name}.log").write_text(result.stdout)
        if (result.returncode == 0) != expected_success:
            raise RuntimeError(f"{name}: exit {result.returncode}\n{result.stdout[-4000:]}")
        return result.stdout

    def configure(name, expected, extra=(), build=None):
        build = build or output / name
        log = run(name, ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja", *extra])
        assert (build / "selected.txt").read_text() == ("TRUE" if expected else "FALSE"), log
        run(name + "-build", ["cmake", "--build", str(build), "-j4"])
        if expected:
            run(name + "-test", ["ctest", "--test-dir", str(build), "--output-on-failure"])
        else:
            targets = run(name + "-targets", ["cmake", "--build", str(build), "--target", "help"])
            assert "glfw" not in targets and "imgui" not in targets, targets
            assert "Skipping dev_sandbox" in log or "BUILD_DEV_SANDBOX=OFF" in log, log
        print(f"PASS {name}: {'Wayland' if expected else 'skipped'}", flush=True)
        return log

    @contextmanager
    def hidden_programs(*names):
        paths = {Path(path) for name in names if (path := shutil.which(name))}
        moved = []
        try:
            for path in paths:
                hidden = path.with_name(path.name + ".sandbox-test-hidden")
                assert not hidden.exists()
                path.rename(hidden)
                moved.append((path, hidden))
            yield
        finally:
            for path, hidden in reversed(moved):
                hidden.rename(path)

    reused = output / "reused"
    configure("installed", True)
    configure("stale-glfw-flags", True,
              ["-DGLFW_BUILD_WAYLAND=OFF", "-DGLFW_BUILD_X11=ON"], reused)
    configure("off", False, ["-DBUILD_DEV_SANDBOX=OFF"], reused)
    configure("on", True, ["-DBUILD_DEV_SANDBOX=ON"], reused)

    if args.remove_packages:
        if not Path("/.dockerenv").exists() or os.geteuid() != 0:
            parser.error("--remove-packages requires a disposable root Docker container")
        with hidden_programs("wayland-scanner"):
            configure("missing-scanner", False, build=reused)
        with hidden_programs("pkg-config", "pkgconf"):
            configure("missing-pkg-config", False, build=reused)
        run("remove-wayland", ["apt-get", "remove", "-y", "libwayland-dev"])
        configure("missing-wayland", False)
        configure("cached-wayland-removed", False, build=reused)
        configure("off-without-wayland", False, ["-DBUILD_DEV_SANDBOX=OFF"])


if __name__ == "__main__":
    main()
