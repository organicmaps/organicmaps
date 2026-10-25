# Water distance measurement

Android Water measures distance along mapped water geometry. Water portions are solid;
a necessary land transfer is dashed. Transfer icons mark both ends of each internal
walking transfer, where the route leaves and rejoins the water. Walking access to
start, finish and intermediate land checkpoints does not create transfer icons.
Icons are cleared when the route is cancelled, recalculated or its mode changes; nearby icons may
be hidden at overview scales to keep the map readable. Results show water distance and, when present,
land distance and their sum in application units. There is no navigation, ETA, speed
or elevation chart. Ruler still measures straight segments.

Select an explicit start with Route from, choose Water in the toolbar, then select
a destination and optional intermediate points. Any checkpoint may be on land.
For each off-water checkpoint, the router selects the nearest known water within
1 km and uses ordinary pedestrian routes between that water point and the selected
checkpoint. The start has a walking approach, the finish has a walking exit, and
an intermediate land stop includes both an exit and a return to water. These paths
are dashed and included in land distance; they do not create new transfer points
between water components. If pedestrian routing fails, there is no straight land
fallback. Water checkpoints normally need no walking access. If their water
component cannot be reached after the wider search, a short walking approach or
exit may instead connect them to an eligible river endpoint within 5 km. The
result must still contain a water portion and is marked unconfirmed. An internal
transfer preserving water at both ends takes precedence over this fallback. For
terminal walks, nearby eligible river endpoints are preferred, so a cheaper
mostly walking alternative does not displace the water journey. River taps snap to the displayed
centerline; line selections retain 0.1 m quantization tolerance. Android keeps route
point controls available on land as well as water.

Water remains the preferred travel mode: walking access connects checkpoints to
the water graph, then the router searches that graph. A failed water search is not
replaced with a direct pedestrian route between checkpoints. Access points are
selected by proximity, without comparing every possible water entry or walking
cost; a different access point may yield a better route or a route where the
selected one fails.

## Routing and transfers

WaterRouter implements IRouter without a road vehicle profile. River lines retain
original OpenStreetMap coordinates and node IDs; crossings alone do not join them.
Surface union and triangulation preserve holes. Exact contacts between water surfaces remain connected; dry gaps stay disconnected. Visibility edges require complete
water coverage. River/surface transitions use actual intersections. Distances use
mercator::DistanceOnEarth. Checkpoints retain their selected order.

Connected water paths found in the searched window take precedence over land shortcuts. All land portions,
including checkpoint approaches, exits and transfers between water components, use the
existing pedestrian routing algorithm. Results show its actual walking geometry
and distance as dashed land portions. If walking fails, the leg fails without a
straight-line fallback.

A checkpoint leg may combine several water portions and pedestrian transfers,
including walking access at both ends. A sparse graph connects eligible endpoints
across disconnected water components. Candidate walks are ranked by geometric
distance and checked with the pedestrian router only when selected. An inaccessible
walk is removed and another candidate is tried. The search requires a water portion
and alternates water with walking, rather than replacing the journey with chained
walks. Candidate selection is bounded and does not compare every actual walking
cost, so mixed routes show "Shortest route not confirmed"; neither completeness
nor the shortest combined water-and-walking route is guaranteed.

With source topology, line-only transfers use original source-way endpoints.
Interior vertices, selected points and extraction cuts are not transfer points.
Surface transfers can use actual shoreline vertices in source-topology maps.
Ordinary-map endpoint walks use the complete stored endpoints instead. Their straight distance estimate may
cross other known water bodies because the returned geometry comes from the
pedestrian router. Unknown areas remain excluded. Android hides navigation controls; native following is disabled.

## Ordinary downloaded maps

Water can use an ordinary country MWM without regenerating it. The compatibility
provider reads its best available waterway lines and water-surface triangles,
unions the surfaces, and preserves the islands represented by that geometry.
Distances follow the stored display geometry, which may be simplified relative
to the original OpenStreetMap data.

Original river node IDs are absent. Separate line features never join merely
because their coordinates coincide or cross. River/surface intersections do join.
As a deliberate measurement simplification, bridge, aqueduct and lock restrictions
are ignored in this compatibility mode. This can join an aqueduct to a water
surface it crosses. Explicit tunnels and negative-layer water features are excluded
from water geometry, but may justify a local pedestrian bypass between nearby
stored river endpoints. The actual bypass follows the standard pedestrian router;
it is never a straight line through a tunnel. Ordinary maps can omit tunnels
entirely, so not every underground connection can be recognized.
This mode does not model navigability, water depth, currents or access restrictions.

If no connected water path exists in the compatibility graph, a leg may include
several pedestrian transfers, each only between endpoints of stored river lines. Interior
vertices, checkpoints, extraction cuts and arbitrary lake shores cannot be transfer
points. An endpoint inside a lake cannot move the proposed transfer to its shoreline.
A known tunnel bypass retains the stored endpoints even when they lie inside a
water surface; it does not move them to arbitrary shore points.
Stored feature endpoints need not be original OpenStreetMap way endpoints. Missing
junctions may therefore require a walking transfer or make a route unavailable.
The endpoint restriction applies to internal transfers; pedestrian
access for off-water checkpoints remains available as described above.
These connections do not reconstruct source topology or prove that a portage is
physically necessary. The walking route must be found by the pedestrian router;
the straight-line estimate used to select endpoints is never the returned land path.
Coastline polygons describe land and do not supply ocean water in this mode.

Checkpoints must lie inside known, downloaded map regions. The application
certifies local coverage from packed borders, with a margin for border
simplification and quantization. Uncovered portions of a search window remain
unknown and cannot supply water edges or walking-transfer candidates; an unused
missing corner does not invalidate a route through covered water. A feature
bounding box alone never certifies coverage. Missing borders and exceeded
extraction budgets remain unavailable. The native water-membership query uses
the same coverage check.
Eligibility uses a 50 m margin instead of extracting a full routing window
for a single point. The feature budget counts intersecting water features,
excluding buildings and roads.
Packed borders describe application map regions; they cannot reconstruct exact
original source-extract coverage from an old MWM.

## Previously prepared experimental maps

The runtime can still read an existing optional `water_geometry` section, which
retains original source node IDs and full water-surface rings. It takes precedence
when its certified coverage contains the search window. Unsupported features and
invalid surfaces remain unknown locally. Its internal transfers use original
river endpoints or actual shore points and still require pedestrian routing.

This PR does not change the map generator or add generation flags, collectors,
sidecars, feature affiliation rules or geometry simplification rules. Creating
new optional sections is outside its scope and can be discussed separately if
source topology becomes necessary. Normal downloaded MWMs need no regeneration.
Their missing original node identities cannot be recovered by client-side routing;
endpoint-only pedestrian transfers are a fallback, not reconstructed water links.

## Bounds and limitations

Optional sections require one MWM to cover the extraction rectangle. Ordinary
maps prefer the combined coverage of up to 16 downloaded neighboring regions,
sharing the same total geometry budgets. Their water surfaces are unioned across
file boundaries; separate river lines still lack original node identities.
Uncovered cells remain unknown, even when a displayed feature overlaps them.
For optional sections, the margin follows checkpoint distance and is bounded
between 1 and 30 km. Ordinary maps start with at most 3 km and reduce the margin
gradually when extraction budgets require it. If a water path is unavailable or the local result requires walking,
one retry may extend that margin by 2 km before selecting pedestrian transfers. Antimeridian windows and extraction
failures remain unavailable.

A known connected path may be returned when further visibility optimization would
exceed its budget, or when coverage cannot exclude a shorter alternative. Such
results show "Shortest route not confirmed". Every returned water edge must still
have known water coverage and avoid unknown areas.

After both water searches fail, a leg may combine walking transfers between
eligible stored endpoints even if a water alternative outside the searched window
cannot be excluded. It remains unconfirmed and must pass ordinary pedestrian
routing. Unknown geometry cannot itself justify a transfer, and a candidate
crossing unknown coverage is rejected. Mapped underground breaks can also justify
local transfers between nearby stored endpoints. These rules do not prove that
carrying a boat is physically necessary.

The reader caches one index with at most 65,536 records. Extraction allows 4,096
surface source points and 65,536 triangles, with 16,384 line source points. The graph
allows 131,072 vertices. Lazy A* starts with valid sparse mesh paths and checks
visibility only when it can improve the route. It allows at most 4,000,000 segment
checks and 64,000,000 indexed triangle visits. Interior triangle vertices keep linear mesh edges;
surface visibility needs only reflex shore corners, checkpoints and river points.
River curvature retains all source vertices, or all stored vertices for ordinary maps.
The walking fallback selects at most 16 nearby candidates per endpoint, with at
most four from each other water component. It allows up to 131,072 candidate links,
4,000,000 candidate visits, 4,000,000 cached water connections and 32 rejected
pedestrian candidates per leg. Successfully validated transfers do not consume
that rejection budget; each directed candidate is checked at most once. These limits can leave a valid route undiscovered. Cancellation is
checked during extraction and graph searches. Tessellation is bounded by its input.
Large or complex waterways may be unavailable. Country-scale performance and map
size have not been benchmarked.

## Performance

Road routing loads a prepared routing graph from the MWM and uses bidirectional
A* and, where applicable, precomputed cross-region connections. Water on ordinary
MWMs reconstructs its geometry from display features at request time. It must
union water surfaces and check that each proposed visibility edge stays on water.
Its lazy A* search already uses straight-line lower bounds and a sparse mesh path
as an upper bound; replacing the search alone does not remove geometry costs.

Ordinary-map water features whose bounding boxes cannot overlap or touch are
unioned in separate groups. Potentially intersecting features remain in the same
group. Every input triangle is retained; this changes neither shore detail nor
the union's covered area. It avoids feeding unrelated lakes into one tessellation.

Walking portions use the same pedestrian search and costs. Water opts into reuse
of the decoded road graph across its walking legs, then clears that graph on
completion, cancellation or failure. Routing-option changes invalidate the cache.
Other routing modes retain their existing cache policy. This avoids reloading a
whole region's road joints for every short portage.

The triangle index is balanced after bulk insertion. Tessellation order is spatially
correlated, so incremental KD-tree insertion can otherwise make repeated segment
coverage queries expensive. Rebalancing preserves the same triangles and exact
coverage predicates; it does not simplify shores or change transfer eligibility.
A controlled API 34 x86_64 Debug benchmark on the exact northward Almere case
compared both indexes using the same 11,679 triangles. Two alternating pairs
measured 22.3–29.4 seconds for the old index and 7.0–9.8 seconds after balancing
for the search stage, with unchanged 5,311.6 m water distance and no land transfer.
The geometry extraction stage took 5.1 seconds in that run. These are emulator
measurements, not a guarantee for physical phones or other routes.

Visibility search omits redundant river bends submerged inside water surfaces;
checkpoints, source endpoints, surface portals and shore corners remain eligible.
Optimistic mesh components include unknown-crossing edges for pruning, while
actual paths still exclude those edges. Boundary goals are spaced at roughly
50 m apart. A* searches slightly beyond the destination score to bound water costs
to exits from the extraction window. Settled goals provide exact costs; other
goals use the remaining A* score bound. These bounds exclude shorter outside
excursions even when straight-line distance alone is too optimistic. Unknown
boundary intervals retain geographic bounds instead of interpolated water costs.

## Calculation progress

Android shows a separate Water progress indicator while calculating, without a
Start button. Geometry loading is indeterminate because its total work is not
known beforehand. Graph construction then reports completed work counts; search
uses the same `AStarSubProgress` distance estimate as road routing. The combined
value is monotone within a request and is an estimate of work, not remaining time.
Only a successfully assembled route reports 100. Cancellation, errors and a new
request reset the indicator; queued updates from cancelled requests are discarded.

## Validation

The latest validation covers 106 native routing/provider/async/spatial-index
tests on an API 34 x86_64 emulator and a Samsung SM-G970F (Android 12, API 31),
two transfer-marker/lifecycle tests on both devices, nine MWM integration tests
on the emulator, 40 real-map scenarios on the emulator and five Android UI route cases on the phone,
115 Android app/SDK unit tests and lint. ARM64 and x86_64 native libraries build.
The maps are ordinary catalog 2026-07-14 downloads. The reporting Xiaomi on
Android 16 was not connected; this is not a claim of physical Android 16 testing. Reduced tests cover several portages, walking access at both ends, retrying inaccessible walking candidates,
cancellation, and rejecting walking-only substitutes for water journeys. Exactly
zero-area clipped triangles are skipped before winding tests and extraction
budgets; thin valid water geometry is retained.

Tests cover bends, junctions, source identity, checkpoint projection, endpoint-only
gaps, islands, river/lake intersections, numerical regressions, distance totals,
missing coverage, open frontiers, cancellation, budgets, zero ETA, shared route
presentation and unchanged Ruler behavior. MWM integration tests cover ordinary-map
islands, missing source identities, neighboring coverage, underground bypasses,
extraction budgets, degenerate clipping and a pinned real OpenStreetMap river.
That extract retains IDs, versions and timestamps and is attributed to OpenStreetMap contributors under ODbL 1.0.

Native tests run on an Android API 34 x86_64 emulator against actual routing,
indexer and unchanged generator libraries. A CLI harness supplies platform
initialization and JVM attachment shims only; these are not application changes.
`routing_integration_tests/water_geometry_mwm_test.cpp` uses the existing
`TestMwmBuilder` to create ordinary MWMs and checks their runtime interpretation.
The tests moved out of `generator_tests`; production generator code and build
registration match the PR base. Only the test of the removed optional-section
generation pipeline was removed. A replacement test verifies that an already
prepared section is still preferred over display geometry. Runtime section-reader
tests are retained.
The official Flevoland MWM also covers the Almere regression: start
52.3542366,5.18088336 to 52.354523,5.238218 produces 5,612.8 m entirely on water.
The original reported start 52.354258,5.180864 lies 2.7 m outside its stored water
geometry; the initial pedestrian approach now handles such selections. Long-path tests cover
adaptive border windows, lazy visibility, thin dry gaps and numerical cancellation
near zero.
The northward Almere case, 52.36468055555556,5.216444444444445 to
52.39769444444445,5.207547222222223, produces 5,311.6 m entirely on water.
A regression reduced from this map covers river/surface intersections along a
nearly parallel shore, both line and route directions, and indexed geometry.

The eastward Almere case, 52.353942,5.180272 to 52.343854,5.285855,
produces 10,634.8 m entirely on water in the official Flevoland MWM. Its failure
combined a visibility budget limit with an insufficient straight-line frontier
bound. Regressions cover submerged river detail, unrelated frontier components,
water detours to exits, and genuinely cheaper unresolved outside routes.

Android app/SDK unit tests and debug builds run separately. Xcode projects include
the common implementation and tessellation dependencies. iOS UI is deferred; no
Apple-platform build has been run in this Windows environment.

The northeast Almere report uses 52.354844,5.179683 to 52.409733,5.36587.
The start is 98.65 m from the nearest stored water (52.3540683,5.18038481).
Water now delegates this initial approach to the standard pedestrian IndexRouter.
The water search can retain its confirmed path when the visibility limit or
unresolved shorter alternatives prevent a shortest-path certificate. Android shows
that qualification alongside water, land and total distances. The real IndexRouter
test reports 216.193 m of walking and 21,423.6 m of water (21,639.8 m total).
A short control route from the same land start reports 216.193 m walking and
303.541 m water, with shortest-path confirmation retained.
Reported route checks on ordinary version 260714 maps: Roma
(52.354819,5.179565 to 52.328613,5.232172) returns 199.071 m walking and
10,077.1 m water. Arthur (54.614027,18.226024 to 54.606715,18.354427)
returns 13,396 m water with shortest-path qualification on Poland_Pomeranian
Voivodeship. The Polish MWM is checked against its catalog BLAKE3 hash before use.
Both pass in the native real-map harness.

The standalone real-map harness uses no deadline, matching interactive Android requests.

Land-checkpoint regressions cover two land endpoints, a walking exit, an intermediate
land stop, exact checkpoint/subroute endpoints, distance accounting, monotone progress,
failed and cancelled walking exits, and the 1 km access-selection limit.

A real-map land-to-land check on the ordinary Flevoland MWM uses
52.354819,5.179565 to 52.355862,5.185028. The standard pedestrian IndexRouter
provides 199.071 m of approach and 23.0997 m of exit; the water portion is 381.264 m.
Both selected endpoints are retained in the resulting route.

The reported 52.354835,5.17963 to 52.328882,5.202655 route also passes the native
ordinary-map harness (208.933 m walking plus 10,478.4 m water, shortest unconfirmed).
On a connected Samsung SM-G970F, the unchanged 2026.10.24-11 APK and matching
260714 Flevoland map successfully built the exact points after selecting Water
from a pedestrian route, in 42.7 seconds. The manual phone workflow reproduced a failure for the selected Gooimeer feature
center: 52.3549188,5.1797099 to 52.3306528,5.1975481. A rounded shoreline
projection could disconnect access from the surface; the required water detour also
fell outside the initial window. Shore access now moves only toward certified
water, and unavailable searches can retry once in a wider covered window.
The native regression and the physical Samsung SM-G970F running 2026.10.24-14
both return 259.257 m walking and 10,841 m water. The phone calculation took
73.5 seconds; debug version identification now appears before V: Scale.


The Polish regression, 54.09895,17.784224 to 54.093102,17.785852, now passes
with the official Pomeranian MWM: 658.814 m water and 55.4064 m walking. A
negative-layer river feature permits the local bypass without adding underground
water or changing the pedestrian router. The result is shortest-unconfirmed.

The same Polish coordinates also pass on a physical Samsung SM-G970F with
2026.10.24-15: the prior build returned RouteNotFound; the fixed build completes
in 1.3 seconds (1.7 seconds after a normal restart). Android displays 660 m water,
55 m walking and the shortest-path qualification. An activity-task recreation
during deep-link testing temporarily lost the map surface; a normal app restart
restored rendering and the successful route details.

The cross-map Almere regression, 52.375208,5.243226 to 52.323037,5.229683,
uses ordinary 260714 Flevoland and Amsterdam maps. It returns 7,624.11 m water
and 175.728 m pedestrian approach. Both maps must be downloaded; the start and
finish belong to different map regions despite being close to Almere.

Android Debug routing error dialogs offer **Copy** to share all selected coordinates,
app version, routing mode, error code, and map/download context for reproduction.
The snapshot is captured when the error opens; copying does not dismiss it.

The Debug-report regression, 52.35484530,5.17963680 to 52.32768966,5.18361091,
returns 11,781.2 m water and 211.63 m pedestrian approach on ordinary 260714
Flevoland and Amsterdam maps. Combined neighboring coverage is tried before a
clipped single-map window, and the wider retry retains
a bounded geometry budget large enough for this water detour.

### Reported-route regressions

`libs/routing/routing_integration_tests/water_reported_routes_test.cpp` preserves
15 reported failing routes, including exact My position starts, plus 25 broader
long-route and cache-equivalence scenarios. Run with ordinary
downloaded MWMs in `--data_path` and matching application resources. Dutch cases
use Flevoland, North Holland — Amsterdam and Utrecht — Amersfoort from catalog
2026-07-14; Polish cases use Pomeranian Voivodeship. Tests require a successful
Water result containing water distance and selected endpoints within the existing
0.1 m display-geometry tolerance. Land portions use the real pedestrian router.

The broader checks connect previously reported destinations in both directions,
including NorthNear and NorthMiddle to Keersluisplas. All four of those routes
failed before the multiple-transfer fix despite being reachable separately from
the same start. They now combine water and pedestrian sections without changing
the pedestrian router. Further checks follow long continuous sections of the
Vistula, Slupia, Wda, Lupawa, Lage Vaart and Hoge Vaart in both directions and require
zero walking distance. These routes were selected from the real map waterway
geometry. A checkpoint chain crosses three legs and preserves every selected stop.

The Keersluisplas report retains My position start `52.35482820, 5.17957070` and
finish `52.45889844, 5.41338676`. Its finish water component has no eligible river
endpoints in the MWM, so a terminal pedestrian exit connects it to a reachable
river endpoint. The 5 km terminal walking limit concerns endpoint proximity; an
actual pedestrian detour can be longer. Mixed routes show "Shortest route not
confirmed". The wider sample improves regression coverage; it does not establish
that every long journey can be found with incomplete display geometry or bounded
search budgets.

### Long mixed routes and walking-graph reuse

The Pomeranian report retains explicit start `54.12300877, 17.75593190` and finish
`53.84449881, 18.35696959`. The previous 32-check limit counted successful walking
legs and rejected this valid multi-portage path after about 81 seconds on the
API 34 x86_64 emulator. The new search limits rejected candidates instead of
rejecting a route merely because it contains many valid transfers. A reduced
regression requires 40 successful transfers.

The real-map result contains 94.142 km of water and 24.114 km of pedestrian
sections, marked "Shortest route not confirmed". A separate regression compares
the complete output coordinate sequence with and without walking-graph reuse,
using the same routing clock. Both sequences match. In one paired emulator run,
calculation took 40.2 seconds without that cache and 10.2 seconds with it; the
number of regional road-graph loads fell from 35 to one. Reverse-direction coverage
is included. Initial measurements with graph reuse and grouped surface unions
were about 16 seconds in the standalone native harness. Android app requests on
that emulator took 29-34 seconds. Reading the same MWM through the emulated app
storage also increased a native-only run from about 17 to 32 seconds, indicating
a substantial storage-path cost. These are not Xiaomi phone measurements or a
promise for other routes.

Physical-phone verification used a Samsung SM-G970F with Android 12 and APK
2026.10.25-1. The Android Water UI built the exact Pomeranian points in both
directions: native calculation times were 18.6 and 18.3 seconds (about 22 seconds
including UI polling). All 40 real-map scenarios and 103 native tests passed on
that phone; all 37 earlier native-route distances and shortest-path qualifications
matched the preceding results. Rameaugracht, Keersluisplas and the short Polish
underground-bypass case also passed through the UI.

The phone's current UI configuration returns about 34 km water for Keersluisplas
and 680 m for the short Polish case, differing from the native fixture values.
A direct APK 2026.10.24-24 versus 2026.10.25-1 comparison on the same phone and maps
returned those same UI distances in both versions. These differences predate the
walking-cache and surface-union changes.

APK 2026.10.25-2 repeats the five phone UI cases with transfer icons. The long
Pomeranian calculation took 20.6 seconds forward and 19.0 seconds in reverse.
Icons were checked at overview and detailed scales in light and dark map styles,
and disappeared on mode changes, recalculation and cancellation. The final
emulator matrix preserves all 37 earlier route distances and qualifications.
