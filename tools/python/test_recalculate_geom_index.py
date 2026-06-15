import importlib.util
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
import threading
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "recalculate", Path(__file__).with_name("recalculate_geom_index.py")
)
recalculate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recalculate)


class RecalculateIndexTests(unittest.TestCase):
    def test_duplicate_names_get_unique_temporaries_and_shared_styles(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            resources, writable = root / "bundle", root / "data"
            resources.mkdir()
            writable.mkdir()
            (resources / "World.mwm").touch()
            for version in ("1", "2"):
                (writable / version).mkdir()
                (writable / version / "Country with spaces.mwm").touch()
            (writable / "WorldCoasts.mwm").touch()
            barrier = threading.Barrier(2)
            temporaries = []
            calls = []

            def run(args, **kwargs):
                calls.append(args)
                path = next(
                    a.split("=", 1)[1]
                    for a in args
                    if a.startswith("--intermediate_data_path=")
                )
                self.assertTrue(Path(path).is_dir())
                output = next(
                    a.split("=", 1)[1] for a in args if a.startswith("--output=")
                )
                self.assertTrue((Path(path) / output).parent.is_dir())
                self.assertTrue((writable / (output + ".mwm")).is_file())
                temporaries.append(path)
                barrier.wait(timeout=5)
                return subprocess.CompletedProcess(args, 0)

            with (
                patch.object(recalculate.subprocess, "run", side_effect=run),
                patch.object(recalculate.subprocess, "Popen") as relaunch,
            ):
                self.assertEqual(
                    recalculate.main(
                        [
                            str(resources),
                            str(writable),
                            "generator",
                            "app",
                            "--designer=style",
                        ]
                    ),
                    0,
                )
                relaunch.assert_called_once()
            self.assertEqual(len(calls), 2)
            self.assertEqual(len(set(temporaries)), 2)
            for args in calls:
                self.assertIn(f"--data_path={writable}", args)
                self.assertIn(f"--user_resource_path={resources}", args)
            self.assertEqual(
                {a for args in calls for a in args if a.startswith("--output=")},
                {
                    f"--output={Path(version) / 'Country with spaces'}"
                    for version in ("1", "2")
                },
            )
            self.assertTrue(all(not Path(p).exists() for p in temporaries))
            self.assertTrue((resources / "World.mwm").is_file())

    def test_symlinked_writable_root_keeps_relative_paths_and_deduplicates_maps(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            writable = root / "data"
            writable.mkdir()
            version = writable / "260901"
            version.mkdir()
            source = root / "Country.mwm"
            source.touch()
            alias = root / "data-link"
            try:
                alias.symlink_to(writable, target_is_directory=True)
                for name in ("Country.mwm", "Country-copy.mwm"):
                    (version / name).symlink_to(source)
            except (OSError, NotImplementedError) as error:
                self.skipTest(f"Cannot create symlinks: {error}")
            with patch.object(recalculate.subprocess, "run") as run:
                self.assertEqual(recalculate.main([tmp, str(alias), "generator"]), 0)
            run.assert_called_once()
            args = run.call_args.args[0]
            output = next(a.split("=", 1)[1] for a in args if a.startswith("--output="))
            self.assertFalse(Path(output).is_absolute())
            self.assertNotIn("..", Path(output).parts)
            self.assertEqual((alias / (output + ".mwm")).resolve(), source.resolve())

    def test_symlinked_version_directories_are_reindexed_once(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            writable, maps = root / "data", root / "external-maps"
            writable.mkdir()
            maps.mkdir()
            source = maps / "Country with spaces.mwm"
            source.touch()
            try:
                for version in ("260901", "260902"):
                    (writable / version).symlink_to(maps, target_is_directory=True)
            except (OSError, NotImplementedError) as error:
                self.skipTest(f"Cannot create symlinks: {error}")
            with patch.object(recalculate.subprocess, "run") as run:
                self.assertEqual(recalculate.main([tmp, str(writable), "generator"]), 0)
            run.assert_called_once()
            output = next(
                arg.split("=", 1)[1]
                for arg in run.call_args.args[0]
                if arg.startswith("--output=")
            )
            self.assertFalse(Path(output).is_absolute())
            self.assertNotIn("..", Path(output).parts)
            self.assertEqual((writable / (output + ".mwm")).resolve(), source.resolve())

    def test_only_maps_directly_in_valid_version_directories_are_selected(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ("minsk-pass.mwm", "World.mwm", "Country.mwm"):
                (root / name).touch()
            for name in ("styles", "1234567", "１２３", "-1", "1x"):
                directory = root / name
                directory.mkdir()
                (directory / "ignored.mwm").touch()
            version = root / "260901"
            version.mkdir()
            expected = version / "Country.mwm"
            expected.touch()
            for name in ("WorldCoasts.mwm", "WorldCoasts_migrate.mwm", "ignored.MWM"):
                (version / name).touch()
            nested = version / "nested"
            nested.mkdir()
            (nested / "ignored.mwm").touch()
            (version / "directory.mwm").mkdir()
            self.assertEqual(recalculate.find_all_mwms(root), [expected.resolve()])

    def test_bundled_world_is_preserved_and_dated_copy_is_reindexed(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "World.mwm").write_text("bundled World")
            (root / "WorldCoasts.mwm").write_text("bundled coasts")
            dated = root / "260901"
            dated.mkdir()
            (dated / "World.mwm").write_text("editable World")
            with patch.object(recalculate.subprocess, "run") as run:
                self.assertEqual(recalculate.main([tmp, tmp, "generator"]), 0)
            run.assert_called_once()
            self.assertIn(f"--output={Path('260901') / 'World'}", run.call_args.args[0])
            self.assertEqual((root / "World.mwm").read_text(), "bundled World")
            self.assertEqual((root / "WorldCoasts.mwm").read_text(), "bundled coasts")

    def test_failure_does_not_relaunch(self):
        for error in (
            FileNotFoundError("missing generator"),
            subprocess.CalledProcessError(2, "generator"),
        ):
            with self.subTest(error=error), TemporaryDirectory() as tmp:
                root = Path(tmp)
                version = root / "260901"
                version.mkdir()
                (version / "Country.mwm").touch()
                with (
                    patch.object(recalculate.subprocess, "run", side_effect=error),
                    patch.object(recalculate.subprocess, "Popen") as relaunch,
                ):
                    self.assertEqual(
                        recalculate.main([tmp, tmp, "generator", "app"]), 1
                    )
                    relaunch.assert_not_called()


if __name__ == "__main__":
    unittest.main()
