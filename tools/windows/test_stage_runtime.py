import tempfile
import unittest
from pathlib import Path

from stage_runtime import copy_inputs, file_hashes


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


if __name__ == "__main__":
    unittest.main()
