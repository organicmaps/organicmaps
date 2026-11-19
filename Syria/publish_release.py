"""Publishes an app release for the in-app "new version" check of APK installs (ServerUpdateCheck).

usage: python Syria/publish_release.py GOOGLE_APK HUAWEI_APK --notes-ar "…" --notes-en "…" [--min-google CODE]
                                       [--min-huawei CODE] [--dry-run]

Reads versionCode/versionName from each APK (aapt2 from the Android SDK), uploads the APKs and apps/version.json
to https://organicmaps.openstreetmap.sy/apps/ on the VPS (ssh alias "syria-vps"). Users on an older version see
"New version available" with a Download button; below --min-* the update is required. Google Play installs use
Play's own prompt instead, so a Play release needs no step here (but keep the APKs in step with it).
"""
import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SITE = "https://organicmaps.openstreetmap.sy"
REMOTE_DIR = "/var/www/organicmaps-site/apps"
HOST = "syria-vps"


def aapt2() -> str:
    sdk = os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT")
    if not sdk:
        sys.exit("Set ANDROID_HOME to the Android SDK")
    tools = sorted((Path(sdk) / "build-tools").iterdir())
    for name in ("aapt2.exe", "aapt2"):
        if (tools[-1] / name).exists():
            return str(tools[-1] / name)
    sys.exit("aapt2 not found in the newest build-tools")


def badging(apk: Path) -> tuple[int, str]:
    out = subprocess.run([aapt2(), "dump", "badging", str(apk)], capture_output=True, text=True, check=True).stdout
    m = re.search(r"versionCode='(\d+)' versionName='([^']+)'", out)
    if not m:
        sys.exit(f"No version in {apk}")
    return int(m.group(1)), m.group(2)


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8")  # Arabic notes on a Windows console
    p = argparse.ArgumentParser()
    p.add_argument("google_apk", type=Path)
    p.add_argument("huawei_apk", type=Path)
    p.add_argument("--notes-ar", default="")
    p.add_argument("--notes-en", default="")
    p.add_argument("--min-google", type=int, default=0)
    p.add_argument("--min-huawei", type=int, default=0)
    p.add_argument("--dry-run", action="store_true")
    a = p.parse_args()

    release = {"notes": {"ar": a.notes_ar, "en": a.notes_en}}
    uploads = []
    for flavor, apk, minimum in (("google", a.google_apk, a.min_google), ("huawei", a.huawei_apk, a.min_huawei)):
        code, name = badging(apk)
        # A fixed name per flavor: the website link never changes; the version is in version.json.
        remote_name = f"SyriaOrganicMaps-{flavor}.apk"
        release[flavor] = {"versionCode": code, "versionName": name, "minVersionCode": minimum,
                           "url": f"{SITE}/apps/{remote_name}"}
        uploads.append((apk, remote_name))

    text = json.dumps(release, ensure_ascii=False, indent=2)
    print(text)
    if a.dry_run:
        return
    with tempfile.TemporaryDirectory() as tmp:
        version_file = Path(tmp) / "version.json"
        version_file.write_text(text + "\n", encoding="utf-8")
        subprocess.run(["ssh", HOST, f"mkdir -p {REMOTE_DIR}"], check=True)
        # APKs first, version.json last: nobody is offered a file that is not there yet.
        for apk, remote_name in uploads:
            subprocess.run(["scp", "-q", str(apk), f"{HOST}:{REMOTE_DIR}/{remote_name}.tmp"], check=True)
            subprocess.run(["ssh", HOST, f"mv {REMOTE_DIR}/{remote_name}.tmp {REMOTE_DIR}/{remote_name}"], check=True)
        subprocess.run(["scp", "-q", str(version_file), f"{HOST}:{REMOTE_DIR}/version.json"], check=True)
        subprocess.run(["ssh", HOST, f"chmod 644 {REMOTE_DIR}/*"], check=True)
    print(f"Published: {SITE}/apps/version.json")


if __name__ == "__main__":
    main()
