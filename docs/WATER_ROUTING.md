# Water route planning and distance measurement

Water on Android and iOS plans a journey along mapped water, using pedestrian
routes for necessary land sections. Water is solid blue; walking is dashed purple. Transfer
icons mark where an internal portage leaves and rejoins the water. Ordinary
walking access to start, finish and intermediate checkpoints does not create
portage icons. Nearby icons may be hidden at overview scales.

The result shows water, walking and total distance in application units. Water
has a calculation indicator but no Start button, navigation, ETA or elevation
chart. The kayak icon follows Ruler on both platforms and uses the same selected
and unselected theme colors as the other routing profiles. Ruler continues to
measure straight segments.

## Checkpoints and walking

Select a start with **Route from**, choose **Water**, and select a destination
and optional intermediate points. Any checkpoint may be on land; the selected
order is preserved. River selections snap to the displayed centerline.

For an off-water checkpoint, the router selects the nearest known water within
1 km and calculates the approach or exit with the existing pedestrian router.
An intermediate land stop includes both the exit and the return to water.
Access-point selection uses proximity rather than comparing all possible walking
routes, so another entry may succeed where the nearest one fails.

Connected water takes priority over walking shortcuts. If the local water search
fails or proposes a portage, the search expands where reachable water meets
its extraction boundary before choosing land transfers. A journey can include multiple portages, each using the pedestrian
router's actual geometry and distance. Failed walking candidates are excluded
and alternatives are tried within bounded limits. There is no straight walking
fallback and no replacement of the whole journey with a pedestrian-only route.

On ordinary MWMs, internal transfers use only the endpoints of stored river
lines. Interior vertices, selected checkpoints, extraction cuts and arbitrary
lake shores cannot become internal transfer points. If a water checkpoint's
component remains unreachable, terminal walking access may connect it to an
eligible river endpoint within 5 km. This limit concerns endpoint proximity;
the pedestrian route can be longer. Internal transfers preserving water at both
ends take precedence over terminal walking access.

## Maps and limitations

Normal downloaded country MWMs work without regeneration. The provider reads
their best available waterway lines and water-surface triangles, preserves
represented islands, and joins actual river/surface intersections. Separate
waterway lines join at exactly coincident stored endpoints on the same layer,
including endpoints from neighboring maps. Names need not match, so a tributary
can meet a differently named river. Ordinary maps lack the original OpenStreetMap
node identities: interior crossings, near endpoints and artificial extraction
cuts do not establish a connection. Endpoint walks remain a fallback for missing
connections. This reconstruction uses the attributes retained in the MWM and
cannot recover omitted bridge/aqueduct information.

Bridge, aqueduct and lock restrictions are ignored for these measurements.
Explicit tunnels and negative-layer water features are excluded from water
geometry; nearby river endpoints may be connected by a pedestrian bypass.
Coastline polygons describe land and do not provide ocean water. Their bounding
rectangles are not missing coverage and must not mask independently mapped
rivers or lakes, including near river mouths. Display geometry may be simplified,
and underground connections may be absent from the map.
Water routing does not assess navigability, depth, currents or access restrictions.
A suggested portage does not establish that carrying a boat is physically necessary.

`WaterReported_NerisNemencia` retains the exact explicit start
54.85352075179435, 25.45188769698143 and finish
54.856528849207365, 25.472672134637833 from PR #13968, without intermediate points.
With `Lithuania_East.mwm` from catalog 260714 it requires a wholly water route
(approximately 3.33 km).
Previously, two Nemenčia line features meeting at the same endpoint were treated
as disconnected, producing a 920 m walking detour. Synthetic MWM tests cover
same-layer endpoint joins, cross-map joins, tunnels, different layers, near misses,
interior crossings and extraction cuts.
The long reported route from `54.82539913135153, 25.748180598020554` to
`54.900349439028986, 24.087091237306595` crosses from the Neris to the Kaunas
Reservoir. Adaptive extraction includes the river bends, while transfers across
the disconnected dam area still require a valid pedestrian route. With catalog
260714 and both Lithuanian regions, the regression requires at least 200 km on
water and at most 5 km walking in each direction. The separate confluence control
ends at `54.9005461209, 23.8703662445` and requires zero walking. These cases use
explicit endpoints without intermediate points; shortest-route optimality is
not asserted.

After the coverage, adaptive-window and lazy-transfer changes, macOS ARM64 Debug
passed 453 routing unit tests, 164 storage tests and all 60 Water integration
tests (13 synthetic-MWM and 47 real-map tests), using a 1 MiB stack. The reported
long Neris route measured 271.54 km water / 2.15 km walking forward and
247.62 km water / 0.78 km walking in reverse; the confluence control measured
225.34 km water with no walking. The long reservoir calculations took about
96–97 seconds in this unoptimized Debug build. These are desktop measurements,
not mobile performance guarantees. Android/iOS device UI testing and APK builds
were not repeated for this core-only update.

Checkpoints need downloaded map coverage. Packed map borders certify local
coverage, including unions of neighboring regions; feature bounding boxes do
not. Coarse border cells are only a filter: water segments crossing them receive
finer coverage checks, so an uncovered corner does not mask a covered river.
The union of region borders and local coverage answers are cached. Unknown
portions cannot supply water edges or transfer candidates.

A valid route is shown with **Shortest route not confirmed** if missing coverage,
search limits or candidate selection prevent proving optimality. Mixed routes
also carry this qualification because walking candidates are ranked by geometric
estimates before the pedestrian router calculates their actual paths.

Previously prepared experimental MWMs with an optional `water_geometry` section
remain readable. Such sections preserve source node identities and surface rings
and take precedence when their certified coverage contains the search window.
Their transfers may use original river endpoints or shoreline points, with all
walking still calculated by the pedestrian router. This PR leaves the generator
unchanged and adds no generation flags, collectors or sidecars. Creating new
optional sections is outside its scope.

## Search bounds and performance

Ordinary-map searches start with a local margin of at most 3 km. Extraction may
shrink it when geometry budgets are exceeded. Reachable water at an extraction
boundary triggers directional expansion when both endpoint components reach
the edge of the window (not necessarily the same side), with at most four attempts and a distance-dependent margin
capped at 30 km. If one component is closed inside the window, one local retry
adds at most 2 km before searching for portages; growing the whole river network
cannot connect that component by water. A failed expansion preserves an
already available route. Up to 16 neighboring regions can participate. Optional-section searches require
one section to cover the window and use a distance-dependent margin capped at
30 km. Antimeridian windows and oversized extractions are unsupported.

Standard-map extraction and graph construction share capacity limits (524,288
graph vertices, 262,144 triangles and 65,536 water features). Line accounting
uses vertices retained after clipping, including new boundary intersections,
rather than charging the entire source line outside the query window. Large
multi-region river routes therefore use the same capacity at both stages.
Walking connections are generated in distance-ordered batches when the search
needs them. A neighborhood is exhausted before the search widens it; a batch
does not permanently limit the available landings. Exact
water distances and pedestrian routes are evaluated for promising paths;
successful and rejected pedestrian checks are reused within a calculation.
Geometry, visibility and candidate budgets still bound memory and work. These
limits can leave a valid route undiscovered. The walking fallback allows up to
32 rejected pedestrian candidates per leg; successful transfers do not consume
that limit. Cancellation is checked during extraction and search.

Unlike road routing, ordinary-map Water constructs its graph from display
geometry at request time. It uses sparse mesh paths and lazy A* visibility
checks, rather than testing every possible pair of water vertices. Spatial
indexes are balanced after bulk insertion. Disjoint surface groups are unioned
separately without simplifying their geometry.

Within one Water request, pedestrian legs reuse the decoded road graph. The
walking algorithm and costs are unchanged; the cache is cleared on completion,
failure or cancellation and invalidated by routing-option changes. Other modes
retain their existing cache policy.

Geometry loading shows indeterminate progress. Graph construction reports work
counts, and search uses the same distance-based progress estimate as road
routing. Progress is monotone within a request, estimates work rather than time,
and reaches 100 only on success. Cancelled requests cannot update a replacement
request's progress.

## Reproduction and tests

`libs/routing/routing_integration_tests/water_reported_routes_test.cpp` retains
the reported failing coordinates and My position flags, long routes in both
directions, continuous-water controls and intermediate stops. Its 47 tests make
48 route calculations: the Pomeranian regression runs with and without walking
graph reuse and compares the complete coordinate sequence. No reported route
was removed when this duplicate calculation was consolidated.

Run these integration tests with ordinary MWMs in `--data_path` and matching
application resources in `--user_resource_path`. The pinned reference catalog is
2026-07-14. Dutch cases use `Netherlands_Flevoland`,
`Netherlands_North Holland_Amsterdam` and `Netherlands_Utrecht_Amersfoort`;
Polish cases use `Poland_Pomeranian Voivodeship`; the cross-region case also
requires `Poland_Kuyavian-Pomeranian Voivodeship`. The Nemenčia regression uses
`Lithuania_East`; the long Neris cases use `Lithuania_East` and
`Lithuania_West`.

Useful phone checks include:

| Start | Finish | Purpose |
| --- | --- | --- |
| 54.82539913135153, 25.748180598020554 | 54.900349439028986, 24.087091237306595 | Reported Neris–Kaunas Reservoir route, no intermediate points; also check reverse direction |
| 54.82539913135153, 25.748180598020554 | 54.9005461209, 23.8703662445 | Water-only control to the Neris/Nemunas confluence |
| 53.09396078, 18.16890286 | 54.30863264, 18.93380513 | Cross-region water-only route; also check reverse direction |
| 54.12300877, 17.75593190 | 53.84449881, 18.35696959 | Long mixed route; also check reverse direction |
| 54.09895, 17.784224 | 54.093102, 17.785852 | Short pedestrian bypass and both transfer icons |
| 52.3542366, 5.18088336 | 52.354523, 5.238218 | Rameaugracht continuous-water control |
| 52.35482820, 5.17957070 | 52.45889844, 5.41338676 | Land approach and terminal walking access |
| 52.354819, 5.179565 | 52.355862, 5.185028 | Both checkpoints on land |

Deterministic routing/provider tests cover geometry, islands, source identities,
endpoint restrictions, missing coverage, numerical precision, budgets,
cancellation, walking failures, distances, progress and transfer markers. They
also cover border cells that partially overlap missing coverage, directional
window growth, 10,000 unrelated waterways, further landings after rejected
candidates, exact water costs behind lower bounds and directed walking when
searching from the finish.
`water_geometry_mwm_test.cpp` uses the existing `TestMwmBuilder` and unchanged
generator to verify ordinary-map extraction, cross-region coverage and optional
section compatibility. Spatial-index regressions cover insertion, rebalancing
and clearing of 22,000 ordered nodes without recursive stack growth. Android
tests cover controls, progress and diagnostics.
iOS tests cover Water selection and core mapping, separate distances, the
conditional shortest-route qualification, progress reset and hidden Start controls.

Before the capacity update, on macOS ARM64, the Debug builds passed 385 routing unit tests, 31 selected map
tests, seven spatial-index tests and all 47 Water integration tests (eight MWM
tests plus the 39 real-map tests). The four ordinary MWMs matched the sizes and
hashes in catalog 2026-07-14. Android passed 122 app/SDK unit tests and lint; iOS
passed 34 routing, settings and presentation tests. The original five routes in the table,
including the reverse long route, were also calculated through the Android
API 29 ARM64 emulator and iOS 18.6 iPhone 16 Pro simulator interfaces. Both showed
approximately 94 km water and 24 km walking on the long route.

A connected iPhone 8 running iOS 16.7.16 also completed all six original table directions
with the same ordinary maps, showing 58 mi water and 15 mi walking on the long
route in both directions. Device UI checks covered cancellation, switching to
pedestrian routing, portrait/landscape layout, light/dark themes and rebuilding
with an intermediate land point. Missing-water errors showed no stale distances
or Start button. The local development build did not include CarPlay or iCloud; those features were not tested.

On APK 2026.10.25-2, a Samsung SM-G970F (Android 12) built the long Pomeranian
route in both directions, with approximately 94 km water and 24 km walking.
Native calculation took 20.6 seconds forward and 19.0 seconds in reverse.
Transfer icons were checked in light/dark styles, at overview/detail scales,
and after rebuilding, mode changes and cancellation. These are measurements
for that phone and map, not performance guarantees.

Android Debug error dialogs provide **Copy** for the selected coordinates,
My position flags, app version, mode, error code and map/download context.
The snapshot is captured when the error opens; copying does not dismiss it.
The Debug overlay shows the app version before scale/FPS information.

The reported cross-region Polish route from `53.09396078, 18.16890286` to
`54.30863264, 18.93380513` is retained in `WaterReported_KuyavianPomeranianLong`,
with its reverse in `WaterLong_KuyavianPomeranianReverse`. Both use ordinary
2026-07-14 maps for Poland's Kuyavian-Pomeranian and Pomeranian voivodeships,
explicit endpoints and no intermediate points, and require zero walking distance.

After the capacity and coastline fixes, all 92 Water unit tests (with a 1 MiB
stack) and 53 Water integration tests (ten MWM and 43 real-map tests) passed on
macOS ARM64. APK `2026.10.30-3-FDroid-debug` passed ten directions in the Android
API 29 ARM64 emulator: all eight table directions and the Vistula-mouth report
below in both directions. The cross-region route measured 168.6 km water and
the Vistula-mouth route 170.7 km water, each with zero walking in both directions.
The earlier Pomeranian mixed route measured approximately 97 km water and
5–6 km walking. Long-route optimality remained unconfirmed. This update was
not tested on the reporting Xiaomi or rerun on iOS.

The code-6 report from `53.09858890, 18.15192177` to
`54.33892355, 18.94185916` is retained in `WaterReported_VistulaMouth` and
`WaterLong_VistulaMouthReverse`, using the same two Polish maps, explicit
endpoints and no intermediate points. Both require zero walking distance.
`WaterMwm_CoastlineDoesNotMaskMappedWater` separately checks mapped water inside
a coastline land polygon and rejects routes onto land or unsupported sea.
