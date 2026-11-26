"""Exercise the real Designer style compiler on every desktop platform, including Windows."""

from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMPILER = ROOT / "tools/kothic/src"


class DesignerStyleGenerationTests(unittest.TestCase):
    def run_tool(self, script, *args):
        run = subprocess.run(
            [sys.executable, str(COMPILER / script), *map(str, args)],
            capture_output=True,
            text=True,
        )
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)

    def test_default_family_matches_bundled_rules(self):
        with TemporaryDirectory(prefix="designer-style-") as tmp:
            output = Path(tmp) / "data"
            output.mkdir()
            styles = output / "styles"
            # The compiler rewrites priority files; keep the checkout and SVG sources untouched.
            shutil.copytree(
                ROOT / "data/styles",
                styles,
                ignore=shutil.ignore_patterns("out", "symbols"),
            )
            for name in (
                "mapcss-mapping.csv",
                "mapcss-dynamic.txt",
                "colors.txt",
                "patterns.txt",
            ):
                shutil.copyfile(ROOT / "data" / name, output / name)
            for theme in ("light", "dark"):
                self.run_tool(
                    "libkomwm.py",
                    "-s",
                    styles / "default" / theme / "style.mapcss",
                    "-o",
                    output / ("default_" + theme),
                    "-p",
                    styles / "default/include",
                    "-d",
                    output,
                )
            self.run_tool(
                "merge_variants.py",
                output / "drules_default",
                "light",
                output / "default_light.bin",
                "dark",
                output / "default_dark.bin",
            )
            self.assertEqual(
                (output / "drules_default.bin").read_bytes(),
                (ROOT / "data/drules_default.bin").read_bytes(),
            )
            for name in (
                "classificator.txt",
                "types.txt",
                "colors.txt",
                "patterns.txt",
            ):
                self.assertEqual(
                    (output / name).read_text(encoding="utf-8"),
                    (ROOT / "data" / name).read_text(encoding="utf-8"),
                    name,
                )


if __name__ == "__main__":
    unittest.main()
