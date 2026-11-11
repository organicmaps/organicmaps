"""Run the real sandbox under headless Weston and retain renderer logs."""

import argparse
import os
import signal
from pathlib import Path
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--resources", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    for scale in (1, 2):
        with tempfile.TemporaryDirectory(prefix="sandbox-display-") as runtime:
            env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", XDG_RUNTIME_DIR=runtime)
            for name in ("DISPLAY", "XDG_SESSION_TYPE"):
                env.pop(name, None)
            env["WAYLAND_DISPLAY"] = "sandbox-smoke"
            name = f"wayland-scale{scale}"
            data = output / (name + "-data")
            data.mkdir(exist_ok=True)
            app = [str(args.binary.resolve()), "--smoke_test",
                   "--resources_path=" + str(args.resources.resolve()),
                   "--data_path=" + str(data), "--log_abort_level=error"]
            with (output / (name + "-renderer.log")).open("w") as renderer_log, \
                    (output / (name + "-display.log")).open("w") as display_log:
                compositor = subprocess.Popen(
                    ["weston", "--backend=headless-backend.so", "--use-pixman",
                     "--width=1600", "--height=1200", f"--scale={scale}",
                     "--socket=sandbox-smoke", "--idle-time=0", "--no-config"],
                    env=env, stdout=display_log, stderr=subprocess.STDOUT)
                try:
                    for _ in range(100):
                        if (Path(runtime) / "sandbox-smoke").exists():
                            break
                        if compositor.poll() is not None:
                            raise RuntimeError("Weston failed; see " + str(display_log.name))
                        time.sleep(0.05)
                    if not (Path(runtime) / "sandbox-smoke").exists():
                        raise RuntimeError("Weston did not create its display socket")
                    process = subprocess.Popen(app, env=env, stdout=renderer_log,
                                               stderr=subprocess.STDOUT, start_new_session=True)
                    try:
                        status = process.wait(timeout=75)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
                        raise
                finally:
                    compositor.terminate()
                    try:
                        compositor.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        compositor.kill()
                        compositor.wait()
            log = Path(renderer_log.name).read_text()
            if status != 0 or "Sandbox graphics smoke test passed" not in log:
                raise RuntimeError(f"Sandbox smoke test failed; see {renderer_log.name}")
            if "VUID-" in log or "Validation Error" in log:
                raise RuntimeError(f"Vulkan validation failed; see {renderer_log.name}")
            if f"ImGui scale {scale} {scale}" not in log:
                raise RuntimeError(f"Unexpected framebuffer scale; see {renderer_log.name}")
            print(f"PASS {name}: maps, API switches, resize and shutdown", flush=True)


if __name__ == "__main__":
    main()
