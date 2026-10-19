"""Pure fixture/parser checks; no adb, emulator, SDK or captured files required.

python3 tools/python/test_drape_queue_stress.py
"""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
import zipfile
from unittest import mock

import drape_queue_stress as stress


def counts():
    return {"size": 5, "peak": 8, "enqueued": 10, "popped": 3, "filtered": 2, "rejected": 0, "cleared": 0}


def snapshot(renderer="backend", stamp=110):
    return {"uptime_s": stamp, "data": {"renderer": renderer, "size": 5, "peak": 10,
                                       "types": {"MapShapeReaded": counts()}}}


class FixtureTest(unittest.TestCase):
    def test_workload(self):
        scenarios = json.loads(stress.workload())["scenarios"]
        self.assertEqual([x["name"] for x in scenarios], list(stress.PHASES))
        for scenario in scenarios:
            steps = scenario["steps"]
            self.assertEqual(len(steps), 1004)
            self.assertEqual(sum(s.get("timeMs", 0) for s in steps), 40000)
            self.assertEqual(sum(s["actionType"] == "centerViewport" for s in steps), 502)
            self.assertFalse(any(s.get("animated", False) for s in steps))

    def test_random_delays_are_reproducible_and_preserve_viewports(self):
        workload = stress.workload(random_delays=True)
        self.assertEqual(workload, stress.workload(random_delays=True))
        scenarios = json.loads(workload)["scenarios"]
        for scenario, original in zip(scenarios, json.loads(stress.workload())["scenarios"]):
            steps = scenario["steps"]
            self.assertEqual(steps[::2], original["steps"][::2])
            self.assertEqual(steps[1], original["steps"][1])
            self.assertEqual(steps[-1], original["steps"][-1])
            delays = [step["timeMs"] for step in steps[3:-1:2]]
            self.assertEqual(len(delays), 500)
            self.assertTrue(all(50 <= delay <= 500 for delay in delays))
            self.assertGreater(len(set(delays)), 1)
        self.assertGreater(sum(t["duration_s"] for t in stress.scenario_timings(scenarios)), 180)

    def test_limits_use_recorded_batched_reference(self):
        self.assertEqual(stress.LIMITS["frontend_queue_peak"], 141)
        self.assertEqual(stress.LIMITS["backend_queue_peak"], 1722)
        self.assertEqual(stress.LIMITS["rss_peak_mib"], 545.484375)

    def test_ready_workload_uses_distinct_map_wide_targets_in_both_phases(self):
        workload = stress.workload(wait_for_ready=True)
        self.assertEqual(workload, stress.workload(wait_for_ready=True))
        phases = json.loads(workload)["scenarios"]
        targets = []
        for phase in phases:
            centers = [step for step in phase["steps"] if step["actionType"] == "centerViewport"]
            self.assertTrue(all(step["waitForReady"] and not step["animated"] for step in centers))
            motion = centers[1:-1]
            points = [(step["center"]["lat"], step["center"]["lon"]) for step in motion]
            self.assertEqual(len(points), 500)
            self.assertEqual(len(set(points)), 500)
            self.assertGreater(max(p[0] for p in points) - min(p[0] for p in points), 0.4)
            self.assertGreater(max(p[1] for p in points) - min(p[1] for p in points), 0.6)
            expected = ([15] * 500 if phase["name"] == stress.PHASES[0] else
                        [(15, 10, 17)[i % 3] for i in range(500)])
            self.assertEqual([step["zoomLevel"] for step in motion], expected)
            targets.append(points)
        self.assertEqual(targets[0], targets[1])

    def test_distributed_stress_preserves_ready_targets_without_barriers(self):
        ready = json.loads(stress.workload(wait_for_ready=True))["scenarios"]
        timed = json.loads(stress.workload(distributed_points=True))["scenarios"]
        for phase, synchronized in zip(timed, ready):
            steps = phase["steps"]
            centers = [step for step in steps if step["actionType"] == "centerViewport"]
            expected = [{key: value for key, value in step.items() if key != "waitForReady"}
                        for step in synchronized["steps"] if step["actionType"] == "centerViewport"]
            self.assertEqual(centers, expected)
            self.assertEqual([step["timeMs"] for step in steps[1::2]], [5000] + [50] * 500 + [10000])

    def test_millisecond_clock_anchor(self):
        anchor = stress.clock_anchor("nowRTC=1767225700500=2026-01-01 00:01:40 nowELAPSED=+1m40s500ms")
        self.assertEqual(anchor["elapsed_s"], 100.5)
        self.assertAlmostEqual(stress.log_uptime("01-01 00:01:42.750  1 2 I tag: message", anchor), 102.75)

    def test_clock_anchor_android_formats(self):
        for api, local, elapsed in ((23, "2026-01-01 00:01:40", "+1m40s250ms"),
                                    (26, "2026-01-01 00:01:40", "100250"),
                                    (34, "2026-01-01 00:01:40.250", " 100250 \r")):
            with self.subTest(api=api):
                anchor = stress.clock_anchor(f"nowRTC=1767225700250={local} nowELAPSED={elapsed}\n")
                self.assertEqual(anchor["elapsed_s"], 100.25)
                self.assertEqual(anchor["rtc_epoch_s"], 1767225700.25)
                self.assertAlmostEqual(stress.log_uptime("01-01 00:01:42.750  1 2 I tag: message", anchor), 102.75)

    def test_proc_stat_with_spaces_and_parentheses(self):
        fields = ["0"] * 20
        fields[0], fields[11], fields[12], fields[19] = "R", "123", "45", "678"
        self.assertEqual(stress.process_stat("12 (a ( b)) " + " ".join(fields)), {"ticks": 168, "start_ticks": 678})

    def test_disjoint_motion_windows_do_not_create_false_stalls(self):
        frames = [{"present_ns": int(stamp * 1e9)} for stamp in (100, 100.1, 200, 200.1)]
        result = stress.metrics([], frames, [(100, 100.2), (200, 200.2)])
        self.assertAlmostEqual(result["interval_max_ms"], 100)
        self.assertEqual(result["gaps_over_250ms"], 0)


class ChunkTest(unittest.TestCase):
    def setUp(self):
        data = snapshot()["data"]
        self.first = dict(data, sample=1, part=0, parts=2)
        self.second = dict(data, sample=1, part=1, parts=2, types={"OverlayMapShapeReaded": counts()})

    def test_single_part_snapshot(self):
        decoder = stress.QueueDecoder()
        data = dict(snapshot()["data"], sample=1, part=0, parts=1)
        self.assertEqual(decoder.feed(data, 110), {"uptime_s": 110, "data": dict(snapshot()["data"], sample=1)})

    def test_reassembly_allows_interleaved_renderers(self):
        decoder = stress.QueueDecoder()
        self.assertIsNone(decoder.feed(self.first, 110))
        frontend = dict(snapshot("frontend")["data"], sample=1, part=0, parts=1)
        self.assertEqual(decoder.feed(frontend, 111)["data"]["renderer"], "frontend")
        complete = decoder.feed(self.second, 110)
        self.assertEqual(set(complete["data"]["types"]), {"MapShapeReaded", "OverlayMapShapeReaded"})
        self.assertFalse(decoder.pending)

    def test_duplicate_conflicting_and_missing_chunks(self):
        for second in (self.first, dict(self.second, size=6), dict(self.second, sample=2),
                       dict(self.second, part=2), dict(self.second, types=self.first["types"])):
            with self.subTest(second=second):
                decoder = stress.QueueDecoder()
                decoder.feed(self.first, 110)
                with self.assertRaises(ValueError):
                    decoder.feed(second, 110)
                self.assertTrue(decoder.pending)

    def test_missing_whole_sample_and_invalid_metadata(self):
        for data in (snapshot()["data"], dict(self.first, sample=2), dict(self.first, parts=0),
                     dict(self.first, part=-1), dict(self.first, sample=True)):
            with self.subTest(data=data), self.assertRaises(ValueError):
                stress.QueueDecoder().feed(data, 110)


class CaptureValidationTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.output = Path(self.directory.name)
        self.scenario = json.loads(stress.workload())
        self.metadata = {"finished_marker": True, "clock_drift_ms": 0, "warnings": [], "collection_errors": [],
                         "markers": [{"kind": kind, "name": name, "uptime_s": stamp}
                                     for name, start, end in ((stress.PHASES[0], 100, 140),
                                                              (stress.PHASES[1], 150, 190))
                                     for kind, stamp in (("started", start), ("finished", end))] +
                                    [{"kind": "benchmark_finished", "uptime_s": 191}]}
        self.samples = [{"uptime_s": stamp, "app": {"start_ticks": 1, "ticks": stamp * 100},
                         "sf": {"start_ticks": 2, "ticks": stamp * 20}, "rss_kib": 300 * 1024,
                         "new_frames": 20} for stamp in range(100, 191)]
        self.frames = [{"present_ns": number * 50000000} for number in range(2000, 3801)]
        self.queues = [snapshot(renderer, stamp) for renderer in ("frontend", "backend") for stamp in (130, 180)]

    def evaluate(self):
        (self.output / "scenario.json").write_text(json.dumps(self.scenario))
        (self.output / "metadata.json").write_text(json.dumps(self.metadata))
        for name in ("samples", "frames", "queues"):
            (self.output / (name + ".jsonl")).write_text("".join(json.dumps(row) + "\n" for row in getattr(self, name)))
        with contextlib.redirect_stdout(io.StringIO()):
            return stress.evaluate(self.output)

    def test_complete_capture_passes(self):
        self.metadata.update(host_capture_duration_s=91, host_wall_duration_s=91.001)
        result = self.evaluate()
        self.assertTrue(result["passed"], result["capture_errors"])
        self.assertAlmostEqual(result["observed"]["overall"]["app_cpu_percent"], 100)
        self.assertAlmostEqual(result["observed"]["motion"]["duration_s"], 50)

    def test_host_sleep_invalidates_otherwise_complete_capture(self):
        self.metadata.update(host_capture_duration_s=91, host_wall_duration_s=532)
        result = self.evaluate()
        self.assertFalse(result["capture_valid"])
        self.assertTrue(any("Host suspend" in error for error in result["capture_errors"]))

    def test_distributed_stress_disables_original_reference_gates(self):
        self.scenario = json.loads(stress.workload(distributed_points=True))
        for sample in self.samples:
            sample["rss_kib"] = 600 * 1024
        result = self.evaluate()
        self.assertTrue(result["capture_valid"], result["capture_errors"])
        self.assertFalse(result["reference_comparable"])
        self.assertIsNone(result["passed"])
        self.assertEqual(result["limits"], {})
        self.assertNotIn("ready_viewports", result)

    def test_random_delays_use_actual_durations_without_reference_gates(self):
        self.scenario = json.loads(stress.workload(random_delays=True))
        timings = stress.scenario_timings(self.scenario["scenarios"])
        self.metadata["markers"] = []
        self.queues = []
        start = 100
        for name, timing in zip(stress.PHASES, timings):
            end = start + timing["duration_s"]
            self.metadata["markers"] += [{"kind": kind, "name": name, "uptime_s": stamp}
                                         for kind, stamp in (("started", start), ("finished", end))]
            self.queues += [snapshot(renderer, end - 10) for renderer in ("frontend", "backend")]
            start = end + 10
        self.metadata["markers"].append({"kind": "benchmark_finished", "uptime_s": end + 1})
        self.samples = [{"uptime_s": stamp, "app": {"start_ticks": 1, "ticks": stamp * 100},
                         "rss_kib": 600 * 1024, "new_frames": 20} for stamp in range(100, int(end) + 2)]
        self.frames = [{"present_ns": stamp * 50000000} for stamp in range(2000, int(end * 20) + 1)]
        result = self.evaluate()
        self.assertTrue(result["capture_valid"], result["capture_errors"])
        self.assertFalse(result["reference_comparable"])
        self.assertIsNone(result["passed"])
        self.assertIsNone(result["reference"])
        self.assertEqual(result["limits"], {})
        self.assertEqual(result["deltas_percent"], {})
        self.assertAlmostEqual(result["observed"]["motion"]["duration_s"],
                               sum(t["duration_s"] - 15 for t in timings))
        self.assertIn("VALID DIAGNOSTIC CAPTURE", (self.output / "result.txt").read_text())
        self.metadata["markers"][1]["uptime_s"] = 140
        result = self.evaluate()
        self.assertFalse(result["capture_valid"])
        self.assertTrue(any("unexpectedly short" in error for error in result["capture_errors"]))

    def test_missing_phase_or_finish_fails(self):
        self.metadata["markers"].pop(1)
        self.assertFalse(self.evaluate()["passed"])

    def synchronized_fixture(self):
        self.scenario = json.loads(stress.workload(wait_for_ready=True))
        self.metadata["clock_anchor"] = stress.clock_anchor(
            "nowRTC=1767225700500=2026-01-01 00:01:40 nowELAPSED=+1m40s500ms")
        self.metadata["markers"][1]["uptime_s"] += 5
        lines = []
        for name, start in zip(stress.PHASES, (105, 150)):
            stamps = [start + 0.01] + [start + 5.01 + index * 0.05 for index in range(1, 501)] + [start + 30.02]
            for index, stamp in enumerate(stamps):
                date = stress.dt.datetime(2026, 1, 1) + stress.dt.timedelta(seconds=stamp)
                data = {"scenario": name, "index": index, "ready": True,
                        "elapsed_ms": 10 if index in (0, 501) else 50}
                lines.append(date.strftime("%m-%d %H:%M:%S.%f") + " 1 2 I tag: DrapeViewport " + json.dumps(data))
        (self.output / "logcat.txt").write_text("\n".join(lines) + "\n")
        return lines

    def test_ready_throughput_excludes_holds_and_has_no_stress_gates(self):
        self.synchronized_fixture()
        result = self.evaluate()
        self.assertTrue(result["capture_valid"], result["capture_errors"])
        self.assertIsNone(result["passed"])
        self.assertFalse(result["reference_comparable"])
        self.assertEqual(result["limits"], {})
        ready = result["ready_viewports"]
        self.assertEqual(ready["completed_viewports"], 1000)
        self.assertAlmostEqual(ready["viewports_per_s"], 20, places=4)
        self.assertAlmostEqual(result["observed"]["motion"]["duration_s"], 50, places=4)
        self.assertEqual(ready["latency_max_ms"], 50)
        # The backend may be idle while the frontend finishes presenting the ready frame.
        self.queues = [snapshot(renderer, stamp) for renderer in ("frontend", "backend") for stamp in (110, 160)]
        self.assertTrue(self.evaluate()["capture_valid"])

    def test_missing_duplicate_failed_or_premature_ready_ack_cannot_pass(self):
        lines = self.synchronized_fixture()
        for invalid in ([], lines[:-1], lines + lines[-1:],
                        [line.replace('"ready": true', '"ready": false') for line in lines],
                        [line.replace('"elapsed_ms": 50', '"elapsed_ms": 1000') for line in lines]):
            with self.subTest(records=len(invalid)):
                (self.output / "logcat.txt").write_text("\n".join(invalid) + "\n")
                result = self.evaluate()
                self.assertFalse(result["capture_valid"])
                self.assertTrue(any("Invalid synchronized capture" in error for error in result["capture_errors"]))

    def test_capture_quality_failures_cannot_pass(self):
        original = copy.deepcopy(self.metadata)
        for key, value in (("clock_drift_ms", 21), ("process_deaths", ["death"]),
                           ("incomplete_queue_chunks", [["backend", 1]]),
                           ("malformed_queue_logs", ["truncated"]), ("unexpected_logcat_exit", {"exit_code": 0}),
                           ("collection_errors", ["poll failed"]), ("finished_marker", False)):
            with self.subTest(key=key):
                self.metadata = dict(original, **{key: value})
                self.assertFalse(self.evaluate()["passed"])

    def test_counter_conservation_and_missing_renderer_fail(self):
        self.queues[0]["data"]["types"]["MapShapeReaded"]["enqueued"] += 1
        self.assertFalse(self.evaluate()["passed"])
        self.queues = [snapshot("frontend", stamp) for stamp in (110, 160)]
        self.assertFalse(self.evaluate()["passed"])

    def test_stale_queue_tail_fails(self):
        self.queues = [snapshot(renderer, stamp) for renderer in ("frontend", "backend") for stamp in (110, 160)]
        self.assertFalse(self.evaluate()["passed"])

    def test_sparse_rss_samples_fail(self):
        for sample in self.samples[1:]:
            del sample["rss_kib"]
        self.assertFalse(self.evaluate()["passed"])

    def test_rss_polling_gaps_fail_including_capture_boundaries(self):
        samples = self.samples
        for polls in ([samples[0], samples[-1]], samples[3:], samples[:-3]):
            with self.subTest(first=polls[0]["uptime_s"], last=polls[-1]["uptime_s"], count=len(polls)):
                self.samples = polls
                result = self.evaluate()
                self.assertFalse(result["passed"])
                self.assertIn("RSS polling has gaps; sampled peak is unreliable", result["capture_errors"])

    def test_app_restart_or_ring_overrun_fails(self):
        self.samples[2]["app"]["start_ticks"] = 2
        self.assertFalse(self.evaluate()["passed"])
        self.samples[2]["app"]["start_ticks"] = 1
        self.samples[2]["new_frames"] = 127
        self.assertFalse(self.evaluate()["passed"])

    def test_memory_and_queue_regressions_fail(self):
        for sample in self.samples:
            sample["rss_kib"] = 600 * 1024
        self.assertFalse(self.evaluate()["passed"])
        for sample in self.samples:
            sample["rss_kib"] = 300 * 1024
        for queue in self.queues:
            if queue["data"]["renderer"] == "backend":
                queue["data"]["peak"] = 1723
        self.assertFalse(self.evaluate()["passed"])

    def test_cpu_and_fps_are_report_only(self):
        self.frames = self.frames[::4]
        for sample in self.samples:
            sample["app"]["ticks"] *= 4
        self.assertTrue(self.evaluate()["passed"])

    def test_existing_output_never_calls_adb_or_overwrites(self):
        apk = self.output / "test.apk"
        apk.touch()
        sentinel = self.output / "input.json"
        sentinel.write_text("preserve this")
        arguments = ["stress", "--apk", str(apk), "--output", str(self.output),
                     "--serial", "emulator-5554", "--reset-benchmark-app"]
        with mock.patch("sys.argv", arguments), mock.patch.object(stress, "tools_from_sdk"), \
                mock.patch.object(stress, "verify_apk",
                                  side_effect=lambda args: setattr(args, "map_version", "260901")), \
                mock.patch.object(stress, "get_map_path", return_value=self.output / "test.mwm"), \
                mock.patch.object(stress, "adb") as adb, contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(stress.main(), 1)
            adb.assert_not_called()
        self.assertEqual(sentinel.read_text(), "preserve this")
        self.assertFalse((self.output / "error.txt").exists())


class InstallSafetyTest(unittest.TestCase):
    def prepare(self, failure=None, installed=True):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        args = SimpleNamespace(output=Path(directory.name), apk=Path("release.apk"),
                               serial="emulator-5554", map_version="260901",
                               map=Path("data/260901/Hong Kong.mwm"))
        calls = []

        def adb(_args, *command, **_kwargs):
            calls.append(command)
            if command[0] == "install":
                if isinstance(failure, Exception):
                    raise failure
                return failure or "Success\n"
            if command[0] == "uninstall":
                return "Success\n"
            shell = command[1] if len(command) > 1 else ""
            # Deliberately differs from the reference emulator: hardware is recorded, not restricted.
            responses = {"getprop ro.kernel.qemu": "1", "id": "uid=0(root) gid=0(root)",
                         "getprop ro.build.version.sdk": "34", "getprop ro.product.cpu.abi": "x86_64",
                         "wm size; wm density": "Physical size: 1080x1920\nPhysical density: 420\n",
                         "cat /sys/devices/system/cpu/present": "0-1", "cat /proc/meminfo": "MemTotal: 1048576 kB\n",
                         "pm path app.organicmaps": "package:/data/app/base.apk" if installed else "",
                         "pm clear app.organicmaps": "Success", "dumpsys package app.organicmaps": "userId=10042"}
            return responses.get(shell, "")

        with mock.patch.object(stress, "adb", side_effect=adb):
            try:
                stress.prepare(args, None)
            except (stress.subprocess.CalledProcessError, RuntimeError) as error:
                return calls, error
        return calls, None

    def test_fresh_apk_and_host_map_replace_benchmark_fixture(self):
        for installed in (False, True):
            with self.subTest(installed=installed):
                calls, error = self.prepare(installed=installed)
                self.assertIsNone(error)
                self.assertEqual([call for call in calls if call[0] == "uninstall"],
                                 [("uninstall", "app.organicmaps")] if installed else [])
                self.assertEqual(sum(call[0] == "install" for call in calls), 1)
                clear = calls.index(("shell", "pm clear app.organicmaps"))
                uploaded = calls.index(("push", "data/260901/Hong Kong.mwm",
                                        "/data/data/app.organicmaps/files/260901/Hong Kong.mwm"))
                self.assertLess(clear, uploaded)

    def test_install_failure_with_zero_or_nonzero_exit_code_stops_setup(self):
        message = "Failure [INSTALL_FAILED_DUPLICATE_PERMISSION]\n"
        for failure in (message, stress.subprocess.CalledProcessError(1, ["adb"], output=message)):
            with self.subTest(failure=failure):
                calls, error = self.prepare(failure)
                self.assertIsNotNone(error)
                self.assertFalse(any(call[0] == "push" or call == ("shell", "pm clear app.organicmaps")
                                     for call in calls))


class ApkInputTest(unittest.TestCase):
    def test_map_path_comes_from_apk_catalog_and_repository_data(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            args = SimpleNamespace(apk=root / "release.apk", aapt="aapt")
            with zipfile.ZipFile(args.apk, "w") as archive:
                archive.writestr("assets/countries.json", json.dumps({"v": 260901}))
            map_path = root / "data/260901/Hong Kong.mwm"
            map_path.parent.mkdir(parents=True)
            map_path.write_bytes(b"fixture")
            with mock.patch.object(stress, "run", return_value="package: name='app.organicmaps' "), \
                    mock.patch.object(stress, "__file__", str(root / "tools/python/drape_queue_stress.py")):
                stress.verify_apk(args)
                self.assertEqual(stress.get_map_path(args.map_version), map_path.resolve())

    def test_missing_or_empty_host_map_fails_before_emulator_access(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            apk = root / "release.apk"
            apk.touch()
            map_path = root / "data/260901/Hong Kong.mwm"
            map_path.parent.mkdir(parents=True)
            for contents in (None, b""):
                with self.subTest(contents=contents):
                    if contents is not None:
                        map_path.write_bytes(contents)
                    args = ["stress", "--apk", str(apk), "--output", str(root / "output")]
                    with mock.patch("sys.argv", args), mock.patch.object(stress, "tools_from_sdk"), \
                            mock.patch.object(stress, "verify_apk",
                                              side_effect=lambda args: setattr(args, "map_version", "260901")), \
                            mock.patch.object(stress, "__file__", str(root / "tools/python/drape_queue_stress.py")), \
                            mock.patch.object(stress, "adb") as adb, \
                            mock.patch.object(stress.subprocess, "Popen") as launch, \
                            contextlib.redirect_stderr(io.StringIO()) as stderr:
                        self.assertEqual(stress.main(), 1)
                        self.assertIn("Required host map is missing or empty:", stderr.getvalue())
                        adb.assert_not_called()
                        launch.assert_not_called()
                    self.assertFalse((root / "output").exists())


class RootSetupTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.args = SimpleNamespace(output=Path(directory.name))

    def test_plain_id_parses_root_and_shell(self):
        self.assertEqual(stress.shell_uid("uid=0(root) gid=0(root) groups=0(root)\n"), 0)
        self.assertEqual(stress.shell_uid("uid=2000(shell) gid=2000(shell)"), 2000)
        self.assertEqual(stress.shell_uid("0\n"), 0)
        self.assertIsNone(stress.shell_uid("id: invalid option -- u"))
        self.assertIsNone(stress.shell_uid(None))

    def test_already_root_does_not_restart_adbd(self):
        with mock.patch.object(stress, "adb", return_value="uid=0(root) gid=0(root)") as adb:
            stress.require_root(self.args)
            adb.assert_called_once_with(self.args, "shell", "id", timeout=5)
        self.assertIn("uid=0(root)", (self.args.output / "root-attempts.txt").read_text())

    def test_root_reconnect_waits_past_old_transport(self):
        replies = ["uid=2000(shell)", "restarting adbd as root\n", "uid=2000(shell)",
                   stress.subprocess.CalledProcessError(1, ["adb"], output="error: device offline"), "uid=0(root)"]
        with mock.patch.object(stress, "adb", side_effect=replies) as adb, \
                mock.patch.object(stress.time, "monotonic", side_effect=range(20)), \
                mock.patch.object(stress.time, "sleep"):
            stress.require_root(self.args)
            self.assertEqual(adb.call_count, 5)
        log = (self.args.output / "root-attempts.txt").read_text()
        self.assertIn("device offline", log)
        self.assertIn("restarting adbd as root", log)
        self.assertTrue(log.endswith("uid=0(root)\n"))

    def test_nonrootable_image_fails_explicitly(self):
        with mock.patch.object(stress, "adb", side_effect=["uid=2000(shell)",
                                                        "adbd cannot run as root in production builds"]):
            with self.assertRaisesRegex(RuntimeError, "cannot run as root"):
                stress.require_root(self.args)
        self.assertIn("production builds", (self.args.output / "root-attempts.txt").read_text())

    def test_root_timeout_is_bounded(self):
        with mock.patch.object(stress, "adb", return_value="uid=2000(shell)"), \
                mock.patch.object(stress.time, "monotonic", side_effect=[0, 1, 31]), \
                mock.patch.object(stress.time, "sleep"):
            with self.assertRaisesRegex(RuntimeError, "within 30 seconds"):
                stress.require_root(self.args)


if __name__ == "__main__":
    unittest.main()
