#!/usr/bin/env python3
"""
Automated App Store Screenshot Generator for Organic Maps (Android).
Runs across supported form factors (Phone, 7-inch tablet, 10-inch tablet) and locales.
"""

import argparse
import os
import subprocess
import sys
import time

FORM_FACTORS = {
    "phone": {"skin": "pixel_7", "width": 1080, "height": 2400, "dpi": 420},
    "seven_inch_tablet": {"skin": "nexus_7", "width": 1200, "height": 1920, "dpi": 320},
    "ten_inch_tablet": {"skin": "nexus_10", "width": 1600, "height": 2560, "dpi": 320},
}

DEFAULT_OUTPUT_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "screenshots", "android")
)


def run_command(cmd, check=True):
    print(f"--> Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(f"Error executing command: {result.stderr}")
        sys.exit(result.returncode)
    return result.stdout.strip()


def build_test_apk():
    print("Building Debug and AndroidTest APKs...")
    android_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "android"))
    gradle_cmd = ["./gradlew", "assembleGoogleDebug", "assembleGoogleDebugAndroidTest"]
    if os.name == "nt":
        gradle_cmd[0] = "gradlew.bat"
    subprocess.run(gradle_cmd, cwd=android_dir, check=True)


def run_tests_and_pull_screenshots(output_dir, locale="en"):
    print(f"Running screenshot test on connected device for locale: {locale}...")
    run_command(["adb", "shell", "setprop", "persist.sys.locale", locale], check=False)

    test_cmd = [
        "adb",
        "shell",
        "am",
        "instrument",
        "-w",
        "-r",
        "-e",
        "class",
        "app.organicmaps.screenshots.ScreenshotGeneratorTest",
        "app.organicmaps.google.debug.test/androidx.test.runner.AndroidJUnitRunner",
    ]
    subprocess.run(test_cmd)

    # Pull screenshots to local output directory
    os.makedirs(output_dir, exist_ok=True)
    device_screenshot_path = "/sdcard/Android/data/app.organicmaps/files/screenshots"
    print(f"Pulling captured screenshots from device to {output_dir}...")
    run_command(["adb", "pull", f"{device_screenshot_path}/.", output_dir], check=False)
    print("Screenshots generation completed successfully!")


def main():
    parser = argparse.ArgumentParser(description="Organic Maps Screenshot Generator")
    parser.add_argument(
        "--output-dir",
        default=DEFAULT_OUTPUT_DIR,
        help="Directory to store captured screenshots",
    )
    parser.add_argument(
        "--locale",
        default="en",
        help="Locale code for generated screenshots (e.g., en, es, fr, de)",
    )
    parser.add_argument(
        "--build-apk",
        action="store_true",
        help="Build APKs before running tests",
    )

    args = parser.parse_args()

    if args.build_apk:
        build_test_apk()

    run_tests_and_pull_screenshots(args.output_dir, args.locale)


if __name__ == "__main__":
    main()
