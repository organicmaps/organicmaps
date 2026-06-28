#!/usr/bin/env python3
"""Convert Android Lint XML report(s) into GitHub Actions annotations.

Emits native ::warning::/::error:: workflow commands so lint issues show up
inline on the PR diff. Unlike a reviewdog PR-review reporter this needs no API
token, so it works identically for pull requests from forks.

Reads every per-module ``lint-results*.xml`` directly rather than a single
merged report: AGP writes each module's report even when its lint task aborts
on an error, so errors get annotated too, and there is no fragile merge step to
malform the XML.

Usage: annotate-android-lint.py [report.xml ...]   (defaults to the glob below)
"""
import glob
import os
import sys
import xml.etree.ElementTree as ET

DEFAULT_GLOB = "android/**/build/reports/**/lint-results*.xml"

# Match the previous reviewdog "level: warning": report Warning and above,
# skipping Information. Lint severities are capitalized in the XML.
SEVERITY = {"Fatal": "error", "Error": "error", "Warning": "warning"}


def enc(s):
    """Escape a string for the data (message) part of a workflow command."""
    return s.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def enc_prop(s):
    """Escape a string for a workflow-command property value (file/title)."""
    return enc(s).replace(":", "%3A").replace(",", "%2C")


def main():
    reports = sys.argv[1:] or sorted(glob.glob(DEFAULT_GLOB, recursive=True))
    if not reports:
        print("No Android Lint reports found; nothing to annotate.", file=sys.stderr)
        return 0

    # GitHub maps annotations by repo-relative path; lint writes absolute paths.
    workspace = os.environ.get("GITHUB_WORKSPACE", "").rstrip("/") + "/"
    seen = set()
    count = 0
    for report in reports:
        try:
            root = ET.parse(report).getroot()
        except (ET.ParseError, OSError) as e:
            # Never fail the job over a single unreadable report.
            print(f"Skipping unreadable lint report {report}: {e}", file=sys.stderr)
            continue
        for issue in root.findall("issue"):
            level = SEVERITY.get(issue.get("severity", "Warning"))
            location = issue.find("location")
            if level is None or location is None or not location.get("file"):
                continue
            path = location.get("file")
            if workspace != "/" and path.startswith(workspace):
                path = path[len(workspace):]
            line = location.get("line", "1")
            col = location.get("column", "1")
            issue_id = issue.get("id", "")
            message = issue.get("message", "")
            # The same issue can appear in more than one report (e.g. a module
            # shared by two app graphs); collapse exact duplicates.
            key = (path, line, col, issue_id, message)
            if key in seen:
                continue
            seen.add(key)
            title = "Android Lint" + (f": {issue_id}" if issue_id else "")
            print(
                f"::{level} file={enc_prop(path)},line={line},col={col},"
                f"title={enc_prop(title)}::{enc(message)}"
            )
            count += 1
    print(
        f"Emitted {count} Android Lint annotation(s) from {len(reports)} report(s).",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
