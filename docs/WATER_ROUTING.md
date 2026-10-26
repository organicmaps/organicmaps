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
fails or proposes a portage, a wider water search is tried before choosing land
transfers. A journey can include multiple portages, each using the pedestrian
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
river features do not join solely because their coordinates coincide or cross:
ordinary maps lack the original OpenStreetMap node identities. Endpoint walks
are a fallback for missing connections, not reconstructed water topology.

Bridge, aqueduct and lock restrictions are ignored for these measurements.
Explicit tunnels and negative-layer water features are excluded from water
geometry; nearby river endpoints may be connected by a pedestrian bypass.
Coastline polygons describe land and do not provide ocean water. Display geometry
may be simplified, and underground connections may be absent from the map.
Water routing does not assess navigability, depth, currents or access restrictions.
A suggested portage does not establish that carrying a boat is physically necessary.

Checkpoints need downloaded map coverage. Packed map borders certify local
coverage, including unions of neighboring regions; feature bounding boxes do
not. Unknown portions of a search window cannot supply water edges or transfer
candidates. A missing, unused corner does not invalidate a known route.

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
shrink it when geometry budgets are exceeded; one wider retry adds up to 2 km.
Up to 16 neighboring regions can participate. Optional-section searches require
one section to cover the window and use a distance-dependent margin capped at
30 km. Antimeridian windows and oversized extractions are unsupported.

Geometry, visibility and walking-candidate budgets bound memory and work. These
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
directions, continuous-water controls and intermediate stops. Its 39 tests make
40 route calculations: the Pomeranian regression runs with and without walking
graph reuse and compares the complete coordinate sequence. No reported route
was removed when this duplicate calculation was consolidated.

Run these integration tests with ordinary MWMs in `--data_path` and matching
application resources in `--user_resource_path`. The pinned reference catalog is
2026-07-14. Dutch cases use `Netherlands_Flevoland`,
`Netherlands_North Holland_Amsterdam` and `Netherlands_Utrecht_Amersfoort`;
Polish cases use `Poland_Pomeranian Voivodeship`.

Useful phone checks include:

| Start | Finish | Purpose |
| --- | --- | --- |
| 54.12300877, 17.75593190 | 53.84449881, 18.35696959 | Long mixed route; also check reverse direction |
| 54.09895, 17.784224 | 54.093102, 17.785852 | Short pedestrian bypass and both transfer icons |
| 52.3542366, 5.18088336 | 52.354523, 5.238218 | Rameaugracht continuous-water control |
| 52.35482820, 5.17957070 | 52.45889844, 5.41338676 | Land approach and terminal walking access |
| 52.354819, 5.179565 | 52.355862, 5.185028 | Both checkpoints on land |

Deterministic routing/provider tests cover geometry, islands, source identities,
endpoint restrictions, missing coverage, numerical precision, budgets,
cancellation, walking failures, distances, progress and transfer markers.
`water_geometry_mwm_test.cpp` uses the existing `TestMwmBuilder` and unchanged
generator to verify ordinary-map extraction, cross-region coverage and optional
section compatibility. Spatial-index regressions cover insertion, rebalancing
and clearing of 22,000 ordered nodes without recursive stack growth. Android
tests cover controls, progress and diagnostics.
iOS tests cover Water selection and core mapping, separate distances, the
conditional shortest-route qualification, progress reset and hidden Start controls.

On macOS ARM64, the Debug builds passed 385 routing unit tests, 31 selected map
tests, seven spatial-index tests and all 47 Water integration tests (eight MWM
tests plus the 39 real-map tests). The four ordinary MWMs matched the sizes and
hashes in catalog 2026-07-14. Android passed 122 app/SDK unit tests and lint; iOS
passed 34 routing, settings and presentation tests. All routes in the table,
including the reverse long route, were also calculated through the Android
API 29 ARM64 emulator and iOS 18.6 iPhone 16 Pro simulator interfaces. Both showed
approximately 94 km water and 24 km walking on the long route.

A connected iPhone 8 running iOS 16.7.16 also completed all six table directions
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
