import os
from pathlib import Path
import platform
import subprocess
from tempfile import TemporaryDirectory
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "unix" / "package_designer.sh"


class PackageDesignerTests(unittest.TestCase):
    def setUp(self):
        self.temp = TemporaryDirectory(prefix="designer-package-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.build = self.root / "build with spaces"
        self.source = self.root / "source"
        self.tools = self.root / "mock-tools"
        self.is_mac = platform.system() == "Darwin"
        self.app_binary = (
            "OrganicMaps.app/Contents/MacOS/OrganicMaps"
            if self.is_mac
            else "OrganicMaps"
        )
        self.executable(self.build / self.app_binary)
        for name in ("generator_tool", "style_tests"):
            self.executable(self.build / name)
        self.write(self.source / "data/styles/default/light/style.mapcss", "style")
        self.write(self.source / "tools/kothic/src/libkomwm.py", "compiler")
        self.write(self.source / "tools/python/stylesheet/drules_info.py", "statistics")
        self.write(self.source / "tools/python/recalculate_geom_index.py", "reindex")
        self.license_files = (
            "LICENSE",
            "NOTICE",
            "DATA_LICENSE.txt",
            "LEGAL",
            "CONTRIBUTORS",
        )
        for name in self.license_files:
            self.write(self.source / name, "license text: " + name)
        self.write(self.source / "LICENSES/ODbL-1.0.txt", "data license")
        self.write(self.source / "LICENSES/Windows/LGPL-3.0-only.txt", "Qt license")

        # Exercise real copying and archiving without external Qt deployment or signing.
        self.executable(
            self.tools / "bin/git",
            'case "$3" in\n'
            "  ls-files) printf '%s\\0' data/styles/default/light/style.mapcss ;;\n"
            "  rev-parse) printf '0123456789abcdef\\n' ;;\n"
            "  *) exit 1 ;;\n"
            "esac\n",
        )
        for name in ("macdeployqt", "codesign"):
            self.executable(self.tools / "bin" / name)
        self.executable(self.tools / "bin/otool", "printf 'minos 14.0\\n'\n")
        self.env = os.environ.copy()
        self.env["OMIM_PATH"] = str(self.source)
        self.env["QT_PATH"] = str(self.tools)
        self.env["PATH"] = str(self.tools / "bin") + os.pathsep + self.env["PATH"]

    @staticmethod
    def write(path, contents):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)

    def executable(self, path, contents="exit 0\n"):
        self.write(path, "#!/bin/sh\n" + contents)
        path.chmod(0o755)

    def run_package(self, parent=None, build=None, cwd=None):
        args = ["bash", str(SCRIPT), str(self.build if build is None else build)]
        if parent is not None:
            args.append(str(parent))
        return subprocess.run(
            args, env=self.env, cwd=cwd, capture_output=True, text=True
        )

    def test_custom_parent_preserves_unrelated_files(self):
        parent = self.root / "Desktop with spaces"
        sentinel = parent / "unrelated-user-file"
        self.write(sentinel, "preserve me")
        package = parent / "OrganicMaps-Designer"
        self.write(package / "stale-file", "old package")
        result = self.run_package(parent)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(sentinel.read_text(), "preserve me")
        self.assertFalse((package / "stale-file").exists())
        self.assertTrue((package / self.app_binary).is_file())
        self.assertTrue((package / "designer.sh").is_file())
        self.assertTrue((package / "data/styles/default/light/style.mapcss").is_file())
        for name in self.license_files:
            self.assertEqual(
                (package / "licenses" / name).read_bytes(),
                (self.source / name).read_bytes(),
            )
        for license_file in (self.source / "LICENSES").rglob("*.txt"):
            relative = license_file.relative_to(self.source)
            self.assertEqual(
                (package / "licenses" / relative).read_bytes(),
                license_file.read_bytes(),
            )

    def test_missing_helper_preserves_existing_package(self):
        parent = self.root / "output"
        sentinel = parent / "OrganicMaps-Designer/keep"
        self.write(sentinel, "previous package")
        for name in ("generator_tool", "style_tests"):
            with self.subTest(helper=name):
                helper = self.build / name
                helper.unlink()
                result = self.run_package(parent)
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                self.assertEqual(sentinel.read_text(), "previous package")
                self.executable(helper)

    def test_default_parent_keeps_build_outputs(self):
        result = self.run_package()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.build / self.app_binary).is_file())
        self.assertTrue((self.build / "OrganicMaps-Designer/designer.sh").is_file())

    def test_invalid_build_directory_does_not_package_the_current_directory(self):
        parent = self.root / "output"
        sentinel = parent / "OrganicMaps-Designer/keep"
        self.write(sentinel, "previous package")
        result = self.run_package(
            parent, build=self.root / "missing-build", cwd=self.build
        )
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(sentinel.read_text(), "previous package")
        self.assertNotIn("Copying", result.stdout)

    def test_missing_license_preserves_existing_package(self):
        parent = self.root / "output"
        sentinel = parent / "OrganicMaps-Designer/keep"
        self.write(sentinel, "previous package")
        (self.source / "DATA_LICENSE.txt").unlink()
        result = self.run_package(parent)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertEqual(sentinel.read_text(), "previous package")


if __name__ == "__main__":
    unittest.main()
