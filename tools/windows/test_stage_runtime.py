import tempfile
import unittest
from pathlib import Path
from unittest import mock

from stage_runtime import (LICENSE_FILES, REQUIRED_RUNTIME_FILES, copy_inputs, file_hashes,
                           stage_runtime, windeployqt_command)


class StageRuntimeTest(unittest.TestCase):
    def test_missing_resource_fails(self):
        with tempfile.TemporaryDirectory() as root:
            source = Path(root) / "source"
            stage = Path(root) / "stage"
            source.mkdir()
            stage.mkdir()
            with self.assertRaisesRegex(ValueError, "missing.txt"):
                copy_inputs(source, stage, resources=("missing.txt",))

    def test_resource_symlink_is_materialized_and_hashed(self):
        with tempfile.TemporaryDirectory() as root:
            source = Path(root) / "source"
            stage = Path(root) / "stage"
            data = source / "data"
            data.mkdir(parents=True)
            stage.mkdir()
            (data / "source.txt").write_bytes(b"map data")
            try:
                (data / "linked.txt").symlink_to("source.txt")
            except OSError:
                self.skipTest("Symlink creation is unavailable")
            for name in ("LICENSE", "NOTICE", "DATA_LICENSE.txt", "LEGAL", "CONTRIBUTORS"):
                (source / name).write_text(name)
            (source / "LICENSES").mkdir()
            (source / "LICENSES" / "Apache-2.0.txt").write_text("license")

            copy_inputs(source, stage, resources=("linked.txt",))

            self.assertFalse((stage / "data" / "linked.txt").is_symlink())
            self.assertEqual((stage / "data" / "linked.txt").read_bytes(), b"map data")
            self.assertEqual(file_hashes(stage)["data/linked.txt"]["size"], 8)

    def test_windeployqt_scans_qml_sources(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            source = root / "source"
            stage = root / "stage"
            command = windeployqt_command(root / "windeployqt.exe", stage,
                                          stage / "OrganicMaps.exe", source)

            self.assertIn("--qmldir", command)
            self.assertEqual(command[command.index("--qmldir") + 1], str(source / "qt" / "qml"))

    def test_qt_quick_runtime_is_required(self):
        for name in ("Qt6Quick.dll", "Qt6QuickWidgets.dll", "qml/QtQuick/Controls/Material/qmldir"):
            self.assertIn(name, REQUIRED_RUNTIME_FILES)

    def test_missing_qml_style_fails(self):
        missing = "qml/QtQuick/Controls/Material/qmldir"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            source = root / "source"
            stage = root / "stage"
            data = source / "data"
            data.mkdir(parents=True)
            (source / "qt").mkdir()
            (source / "qt" / "runtime_resources.txt").write_text("countries.json\n", encoding="utf-8")
            for name in ("countries.json", "icudt78l.dat"):
                (data / name).write_text(name)
            for name in LICENSE_FILES:
                (source / name).write_text(name)
            (source / "LICENSES").mkdir()
            (source / "LICENSES" / "Apache-2.0.txt").write_text("license")
            stage.mkdir()
            exe = stage / "OrganicMaps.exe"
            exe.write_bytes(b"exe")
            windeployqt = root / "windeployqt.exe"
            windeployqt.write_bytes(b"windeployqt")

            def deploy(*args, **kwargs):
                for name in REQUIRED_RUNTIME_FILES:
                    if name != missing:
                        (stage / name).parent.mkdir(parents=True, exist_ok=True)
                        (stage / name).write_bytes(b"")

            with mock.patch("subprocess.run", side_effect=deploy) as run:
                with self.assertRaisesRegex(ValueError, missing):
                    stage_runtime(source, stage, windeployqt, "direct", "1.0", "6.8.0")

            run.assert_called_once_with(windeployqt_command(windeployqt, stage, exe, source),
                                        check=True)


if __name__ == "__main__":
    unittest.main()
