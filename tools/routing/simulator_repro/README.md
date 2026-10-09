# ARM64 iOS simulator reproductions for #13738–#13741

These isolated probes reproduce four routing failures at PR #13518 commit
`7d9cca20d11befd6fe97acbd5b7a065fd3916b54`. They run in a native UIKit app with
the production iOS `Platform` and main dispatch queue. The script compiles
temporary copies of three production sources and links them with the libraries
from a normal Debug OMaps simulator build. It does not edit the checkout.

Verified on 2026-10-09 with Xcode 27.0 (27A266a), iPhone 18 Pro simulator,
iOS 27.0 (24A434), ARM64 Debug. Both executables and the routing archive were
verified to contain only ARM64.

## Run from a clean checkout

Save `main.mm`, `build.py`, and `run.py` together outside the checkout, or copy
this directory out of the reproduction branch. Use a checkout of the exact
commit above with submodules and generated resources configured according to
the project's installation instructions. Select a booted iOS simulator from
`xcrun simctl list devices booted`.

```bash
# Run in the configured Organic Maps checkout. Set the two paths below.
REPRO_DIR=/absolute/path/to/simulator-repro
SIM_UDID=your-booted-ios-simulator-udid

xcodebuild -workspace xcode/omim.xcworkspace -scheme OMaps \
  -configuration Debug -destination "platform=iOS Simulator,id=$SIM_UDID" \
  -derivedDataPath "$REPRO_DIR/derived-data" \
  ARCHS=arm64 EXCLUDED_ARCHS=x86_64 ONLY_ACTIVE_ARCH=YES \
  CODE_SIGNING_ALLOWED=NO MARKETING_VERSION=2026.10.09 \
  CURRENT_PROJECT_VERSION=1 build

python3 "$REPRO_DIR/build.py" --repo "$PWD" \
  --products "$REPRO_DIR/derived-data/Build/Products/Debug-iphonesimulator"
python3 "$REPRO_DIR/run.py" --device "$SIM_UDID"
```

The probes use separate bundle IDs, `app.organicmaps.routing-repro.baseline`
and `app.organicmaps.routing-repro.fixed`. They do not install or change the
Organic Maps app or its data. `run.py` saves full logs beside the scripts and
requires an explicit completion marker: a successful `simctl launch` alone
does not establish that the app's checks passed.

The baseline app asserts that each reported failure occurred. The fixed app
asserts the corrected observations. Consequently both print `PASS` when the
reproduction and the corresponding controls behave as expected.

## Measurements

| Issue | Baseline | Candidate control |
| --- | --- | --- |
| #13738 | NoError, one calculation, one clear, successful result cannot promote its caches | Same result can promote its caches |
| #13739 | Checkpoint callback reports 1; followed cursor goes from 1 to 0 after adoption | Followed cursor stays at 1 |
| #13740 | `FollowRoute()` clears balloons; native main queue then creates 2 while following is true | Queue leaves 0 balloons |
| #13740 reset control | Reset/discard leaves 0 balloons | Still 0 balloons |
| #13741 | Selected DistanceBiased becomes Normal; adjustment remains disabled | DistanceBiased is preserved; adjustment remains disabled |

Captured baseline output:

```text
ISSUE 13738: NoError=1 calculations=1 clears=1 cache_promotable=0
ISSUE 13739: checkpoint_passed=1 cursor_before=1 cursor_after=0
ISSUE 13740: main_queue=1 following=1 marks_after_cleanup=0 marks_after_queue=2
ISSUE 13740 reset_control: main_queue=1 following=0 marks_after_cleanup=0 marks_after_queue=0
ISSUE 13741: selected=DistanceBiased traffic_adjust=0 rebuilt_strategy=Normal
SIMULATOR_REPRO PASS: 4 issues plus reset control
```

Captured candidate-control output:

```text
ISSUE 13738: NoError=1 calculations=1 clears=1 cache_promotable=1
ISSUE 13739: checkpoint_passed=1 cursor_before=1 cursor_after=1
ISSUE 13740: main_queue=1 following=1 marks_after_cleanup=0 marks_after_queue=0
ISSUE 13740 reset_control: main_queue=1 following=0 marks_after_cleanup=0 marks_after_queue=0
ISSUE 13741: selected=DistanceBiased traffic_adjust=0 rebuilt_strategy=DistanceBiased
SIMULATOR_REPRO PASS: 4 issues plus reset control
```

## Reproduction boundaries and code

`main.mm` contains four named probes:

- `ClearGap`: parks the routing worker after `ThreadFunc` unlocks and before
  private `CalculateRoute` snapshots the request. It queues a clear and a
  replacement calculation, then releases the worker. No timing sleeps are
  used to produce the interleaving; a condition variable gates it.
- `CheckpointProgress`: builds synthetic geometry with Mercator points
  `(0, .001)`, `(0, .003)`, `(0, .006)` and accurate cumulative distances.
  GPS fixes enter the real `RoutingSession::OnLocationPositionChanged` at
  `y=.002` and `y=.003`, longitude 0, accuracy 1 metre. The worker is gated
  after capturing checkpoint progress, so the intermediate stop passes before
  its earlier snapshot is adopted.
- `QueuedBalloons`: uses real `RoutingManager`, `BookmarkManager`, and
  `TransitReadManager`. Its successful route callback queues mark creation,
  calls the full production `FollowRoute()`, checks immediate cleanup, and
  posts an observation behind mark creation on the native iOS main queue. A
  separate reset case checks that the existing generation/choice guard works.
- `TrafficSelection`: uses real `RoutingSession`, `AsyncRouter`, and the
  `IndexRouter::SwapAltRouteToActive`/`ClearState` methods. The wrapper creates
  synthetic routes instead of running A*, then records the real IndexRouter
  strategy at the traffic-triggered full calculation.

Test-only header copies expose `RoutingManager` methods and the IndexRouter
strategy to the probe; dependency headers keep their normal access. The
instrumentation hook is inactive outside the clear-gap probe.

This confirms the core failures on the simulator, including actual native
main-queue ordering and the full `FollowRoute()` path. It does not confirm an
end-to-end CarPlay screen interaction, Core Location playback, real-map A*
geometry, live traffic downloads, Android behavior, or physical hardware.

## Candidate changes

`build.py` documents and applies these changes only to temporary source copies:

1. **#13738:** consume `m_clearState` and snapshot `m_hasRequest` under the
   same `m_guard` acquisition in private `CalculateRoute`, removing the
   separated processing in `ThreadFunc`.
2. **#13739:** advance result variants to the latest passed checkpoint before
   `AssignRoute` promotes the active result.
3. **#13740:** require `!m_routingSession.IsFollowing()` in the queued marker
   task, alongside its generation and alternative-count checks.
4. **#13741:** remove the explicit `ClearState()` from
   `RebuildRouteOnTrafficUpdate`; enqueueing the forced full calculation still
   cancels the previous delegate and invalidates the cached generation.

These are validation prototypes, not a production patch. The checkpoint
change validates the partial-progress case; finished journeys, polyline
position, remaining distance, and renderer restoration need coverage. The
clear change also needs coverage of a clear without a request/router and of
worker shutdown. Preserve the default selection when starting a new journey.
