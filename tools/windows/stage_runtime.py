"""Finish a fresh WindowsRuntime install tree after CMake installs OrganicMaps.exe."""

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


LICENSE_FILES = ("LICENSE", "NOTICE", "DATA_LICENSE.txt", "LEGAL", "CONTRIBUTORS")


def copy_inputs(source: Path, stage: Path, resources=None) -> None:
    if resources is None:
        resources = (source / "qt" / "runtime_resources.txt").read_text(
            encoding="utf-8").splitlines() + ["icudt78l.dat"]
    data_source = source / "data"
    missing = [name for name in resources if not (data_source / name).exists()]
    if missing:
        raise ValueError(f"Missing required runtime resources: {', '.join(missing)}")

    data_target = stage / "data"
    data_target.mkdir()
    for name in resources:
        src = data_source / name
        dst = data_target / name
        dst.parent.mkdir(parents=True, exist_ok=True)
        if src.is_dir():
            shutil.copytree(src, dst, symlinks=False)
        else:
            shutil.copyfile(src, dst, follow_symlinks=True)

    licenses = stage / "licenses"
    licenses.mkdir()
    for name in LICENSE_FILES:
        shutil.copyfile(source / name, licenses / name)
    shutil.copytree(source / "LICENSES", licenses / "LICENSES", symlinks=False)


def file_hashes(stage: Path) -> dict:
    files = {}
    for path in sorted(stage.rglob("*")):
        # Keep the published stage self-contained if a future deploy step preserves links.
        if path.is_symlink():
            raise ValueError(f"Staged symlink: {path}")
        if not path.is_file() or path.name == "build-provenance.json":
            continue
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
        files[path.relative_to(stage).as_posix()] = {
            "sha256": digest.hexdigest(),
            "size": path.stat().st_size,
        }
    return files


def stage_runtime(source: Path, stage: Path, windeployqt: Path, channel: str,
                  version: str, qt_version: str) -> None:
    source = source.resolve()
    stage = stage.resolve()
    exe = stage / "OrganicMaps.exe"
    if not exe.is_file() or {p.name for p in stage.iterdir()} != {"OrganicMaps.exe"}:
        raise ValueError("Use a fresh stage prefix containing only the installed OrganicMaps.exe")
    if not windeployqt.is_file():
        raise ValueError(f"Missing windeployqt: {windeployqt}")

    copy_inputs(source, stage)
    subprocess.run(
        [str(windeployqt), "--release", "--no-compiler-runtime", "--no-translations",
         "--dir", str(stage), str(exe)],
        check=True,
    )

    for required in ("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Network.dll",
                     "platforms/qwindows.dll", "tls/qschannelbackend.dll"):
        if not (stage / required).is_file():
            raise ValueError(f"windeployqt did not stage {required}")

    map_version = json.loads((stage / "data" / "countries.json").read_text(encoding="utf-8"))["v"]
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    manifest = {
        "channel": channel,
        "package_version": version,
        "source_commit": commit,
        "qt_version": qt_version,
        "map_data_version": map_version,
        "files": file_hashes(stage),
    }
    (stage / "build-provenance.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--windeployqt", type=Path, required=True)
    parser.add_argument("--channel", choices=("development", "direct", "store"), required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--qt-version", required=True)
    args = parser.parse_args()
    stage_runtime(args.source, args.stage, args.windeployqt, args.channel,
                  args.version, args.qt_version)


if __name__ == "__main__":
    main()
