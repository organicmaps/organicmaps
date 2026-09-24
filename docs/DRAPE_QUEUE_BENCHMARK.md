# Frontend message queue benchmark

`DRAPE_QUEUE_TRACE` measures the **FrontendRenderer message queue**, including its
normal/high-priority deque and low-priority deque. It does not measure render groups,
the backend queue, or tile-reading tasks.

Build both revisions with identical diagnostics and build options:

```sh
cmake --preset debug -B build-queue-trace \
  -DDRAPE_QUEUE_TRACE=ON -DDRAPE_SCENARIO=ON
cmake --build build-queue-trace --target desktop
```

Tracing counts each insertion and removal under the existing queue mutex. The
frontend writes a `DrapeQueue` JSON log record about once per second. `peak` is the
exact high-water mark since renderer creation, including bursts between samples;
`size` is the queue length at the sample. Per-type counters contain:

| Counter | Meaning |
| --- | --- |
| `enqueued` | Messages inserted into either deque |
| `popped` | Messages dequeued, including those subsequently rejected by the renderer |
| `filtered` | Queued messages removed by a queue filter |
| `rejected` | Incoming messages rejected by a persistent filter or singleton deduplication |
| `cleared` | Queued messages removed by `Clear()` |
| `size`, `peak` | Current and maximum count of this type |

For each type, `enqueued = popped + filtered + cleared + size`. Per-type peaks
need not occur simultaneously and must not be summed to obtain the queue peak.
`filtered` counts whole messages; removing entries from a retained overlay or
user-mark batch releases their payloads without decrementing the message count.
Logging copies counters, not queued messages, and scans no payloads. The option is
off by default and adds no counters or logging to ordinary builds.

## Repeatable macOS run

The existing scenario runner, also used by the local `vng-bench` branch, reads
`graphics_benchmark.json` from the settings directory when Drape starts. Build with
`DRAPE_SCENARIO=ON` to enable it. `waitForTime.time` remains seconds; `timeMs` permits
subsecond waits. `centerViewport.animated` defaults to `true` for existing files.

Create an isolated writable directory. Use the same downloaded map versions in
both runs; the example below uses Hong Kong. The map directory must match the `v`
in `data/countries.json`: newer directories are ignored and older maps trigger an
update. Existing world maps are read from the resources directory.

```sh
mkdir -p /private/tmp/drape-queue-before/260714
ln -s "$PWD/data/260714/Hong Kong.mwm" \
  '/private/tmp/drape-queue-before/260714/Hong Kong.mwm'
python3 tools/python/drape_queue_benchmark.py generate \
  --output /private/tmp/drape-queue-before/graphics_benchmark.json \
  --lat 22.302 --lon 114.17 --pan-degrees 0.02
```

The workload alternates same-zoom pans and zoom changes; the generator defaults to
London when coordinates are omitted. Each phase has 5 seconds to warm up, 500 moves
50 ms apart, and 10 seconds to drain. Repeated A→B→A moves exercise leaving and
returning to tile coverage.
Use `--lat`, `--lon`, `--pan-degrees`, `--interval-ms`, and `--steps` to reproduce
other workloads. Maps for every visited point must be installed before recording;
the existing runner otherwise downloads missing regions.

Launch the app directly so its log is captured:

```sh
MWM_RESOURCES_DIR="$PWD/data" \
MWM_WRITABLE_DIR=/private/tmp/drape-queue-before \
  build-queue-trace/OMaps.app/Contents/MacOS/OMaps \
  > /private/tmp/drape-queue-before.log 2>&1
```

The first launch of an isolated directory may require accepting the existing EULA
dialog. Keep the app visible, with the same display, window size, style, and renderer
for both runs. Wait for `Drape benchmark finished` in the log before closing it.
Verify the log contains `Loaded Hong Kong map` and no `DownloadNode()` calls before
using the measurements. The benchmark waits for actual `OnDisk` status if maps
must be downloaded; intermediate download notifications do not start it.
The scenario runner keeps rendering active, so use it identically in both builds.
Launching the native app requires GUI access; these scenarios do not require
Accessibility automation or mouse scripting.

Repeat with a fresh `drape-queue-after` directory and the identical scenario and
map symlinks, then compare:

```sh
python3 tools/python/drape_queue_benchmark.py summarize \
  /private/tmp/drape-queue-before.log /private/tmp/drape-queue-after.log
```

Run several repetitions in alternating before/after order. Compare exact peak,
pending messages after the final drain, and per-type enqueued/popped/filtered
counts. The summary stops at the last sample before `Drape benchmark finished`,
excluding subsequent normal app activity; this sample is about one second before
completion at most while frames continue normally. Use successive samples to see
growth and drain rate. A peak alone cannot show whether the remaining messages
were useful. To investigate CPU time as well,
record the same workload with Instruments Time Profiler; queue logs provide the
counts that sampling the CPU cannot recover.

For a baseline predating this instrumentation, apply only the tracing/scenario
support to that revision and retain its original queue-filtering algorithm. Do
not compare a traced build with an untraced build or carry queue fixes into the
baseline. Record both commit IDs, build mode, map versions, viewport size, and
scenario JSON alongside the logs.
