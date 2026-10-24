# Water distance measurement

Android Water measures distance along mapped water geometry. Water portions are solid;
a necessary land transfer is dashed. Results show water distance and, when present,
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
fallback. Water checkpoints need no walking access. River taps snap to the displayed
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
existing, unchanged pedestrian IndexRouter. Results show its actual walking geometry
and distance as dashed land portions. If walking fails, the leg fails without a
straight-line fallback.

Each checkpoint leg may propose one transfer using a geometric lower bound to
select its endpoints. Other endpoint pairs are not retried if the selected walking
route fails. The selection does not compare actual walking costs for every pair,
so internal transfers show "Shortest route not confirmed"; the shortest combined
water-and-walking route is not guaranteed.

With source topology, line-only transfers use original source-way endpoints.
Interior vertices, selected points and extraction cuts are not transfer points.
Surface transfers use shoreline intersections. Candidates crossing a third
disconnected water component are rejected. Ordinary-map endpoint rules are described
below. Only the internal arrival marker required by shared presentation is produced.
Android hides navigation controls; native following is disabled.

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
stored river endpoints. The actual bypass follows the unchanged pedestrian router;
it is never a straight line through a tunnel. Ordinary maps can omit tunnels
entirely, so not every underground connection can be recognized.
This mode does not model navigability, water depth, currents or access restrictions.

If no connected water path exists in the compatibility graph, a leg may include
one pedestrian transfer only between endpoints of stored river lines. Interior
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

The complete query rectangle must lie inside a known map-region polygon. The
application supplies this proof from packed borders, with a margin for border
simplification and quantization. A feature bounding box alone never certifies
coverage. Missing borders, cross-region queries and exceeded budgets remain
unavailable. The native water-membership query uses the same coverage check.
Eligibility uses a 50 m margin instead of extracting a full routing window
for a single point. The feature budget counts intersecting water features,
excluding buildings and roads.
Packed borders describe application map regions; they cannot reconstruct exact
original source-extract coverage from an old MWM.

## Optional MWM section

An optional water_geometry section retains original source topology and takes
precedence over the compatibility provider. It also permits coverage-checked
transfer candidates; their land portions still use the pedestrian router, with the
endpoint-selection limits described above. Generate these maps with normal resources
and borders:

```sh
generator_tool --preprocess --osm_file_name=complete.osm --osm_file_type=xml \
  --data_path=DATA --intermediate_data_path=TMP/
generator_tool --generate_features --generate_geometry --generate_index \
  --generate_water_routing --water_routing_source_bounds=WEST,SOUTH,EAST,NORTH \
  --osm_file_name=complete.osm --osm_file_type=xml \
  --data_path=DATA --intermediate_data_path=TMP/ --output=COUNTRY
```

Bounds are longitude/latitude degrees declaring complete source coverage, including
full crossing ways and multipolygons. Do not declare an incomplete extract complete.
Only country border rings wholly inside these bounds are certified. The optional
versioned water_geometry section stores coverage polygons, a bounding-box index,
full surface rings and original source node IDs.

A separate collector preserves waterways before display classification, including
culverts omitted from rendering. Bridge/tunnel waterways and nonzero layers are
unsupported and retain local unknown bounds. Invalid surface rings likewise stay
unknown locally. They cannot become water edges or dry transfer gaps. Unrelated
unknown features do not disable a certified short route elsewhere in the window.
Every raw country generation invalidates old sidecars and provenance,
including opt-out runs. Section-only generation requires fresh provenance. Normal
map generation changes only when Water is enabled.

For oceans use the normal --emit_coasts pipeline with validated closed
WorldCoasts.geom cells. Raw coastline rings enclose land and are never treated as
water. Without generated ocean cells coastline queries remain unsupported. Ocean
generation has not been exercised in the Android smoke test.

## Bounds and limitations

One MWM must cover the extraction rectangle. For optional sections, the margin
follows checkpoint distance and is bounded between 1 and 30 km. Ordinary maps start
with at most 3 km and reduce the margin when coverage or extraction budgets require
it. Near a map boundary, each rectangle side expands only within certified coverage.
If a water path is unavailable in that window, one retry may extend the standard-map
margin by 1 km. The wider query still requires certified map coverage and the same
geometry budgets. Cross-MWM routes, antimeridian windows and extraction failures
remain unavailable.
A known connected path may be returned when the visibility budget is exhausted, or
when bounds cannot exclude a shorter alternative outside complete coverage. Such
results explicitly show "Shortest route not confirmed". Every returned water edge must
still have known water coverage and avoid unknown areas. Missing geometry never
becomes a water edge. A reachable graph touching the extraction frontier cannot
establish disconnection, and disconnected components touching unknown geometry
cannot justify a land gap. A mapped tunnel or negative-layer break is an exception:
a local transfer may join stored river endpoints within 50 m of that feature,
provided the candidate crosses it and avoids unknown geometry. Such a result does
not establish that no water alternative exists outside the window and shows
"Shortest route not confirmed". Missing map data alone never permits this exception.

The reader caches one index with at most 65,536 records. Extraction allows 4,096
surface source points and 16,384 triangles, with 16,384 line source points. The graph
allows 65,536 vertices. Lazy A* starts with valid sparse mesh paths and checks
visibility only when it can improve the route. It allows at most 4,000,000 segment
checks and 64,000,000 indexed triangle visits. Interior triangle vertices keep linear mesh edges;
surface visibility needs only reflex shore corners, checkpoints and river points.
River curvature retains all source vertices, or all stored vertices for ordinary maps.
Transfer candidates are bounded to 2,048. Cancellation is
checked during extraction and graph searches. Tessellation is bounded by its input.
Large or complex waterways may be unavailable. Country-scale performance and map
size have not been benchmarked. The synthetic integration section occupies 1,018 bytes.

## Performance

Road routing loads a prepared routing graph from the MWM and uses bidirectional
A* and, where applicable, precomputed cross-region connections. Water on ordinary
MWMs reconstructs its geometry from display features at request time. It must
union water surfaces and check that each proposed visibility edge stays on water.
Its lazy A* search already uses straight-line lower bounds and a sparse mesh path
as an upper bound; replacing the search alone does not remove geometry costs.

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

The latest native run passes 86 routing/provider/async/spatial-index tests, including
replacement of geometric gaps with pedestrian polylines, failed transfers without a straight-line
fallback, and cancellation during a transfer. ARM64 and x86_64 native builds pass,
and the x86_64 APK installs on the emulator. Android validation passed all
113 app/SDK unit tests and lintAllModules; six generator tests also passed.
Initial visual checks were blocked by emulator System UI ANRs; the emulator and
physical-phone checks below were run after the interface recovered.

Tests cover bends, junctions, source identity, checkpoint projection, endpoint-only
gaps, islands, river/lake intersections, numerical regressions, distance totals,
missing coverage, open frontiers, cancellation, budgets, zero ETA, shared route
presentation and unchanged Ruler behavior. Generator tests cover original-node
reconstruction, grade separation, repeat-generation provenance and a pinned real
OpenStreetMap river. That extract retains IDs, versions and timestamps and is
attributed to OpenStreetMap contributors under ODbL 1.0.

Native tests run on an Android API 34 x86_64 emulator against actual routing,
indexer and generator libraries. A CLI harness supplies platform initialization
and JVM attachment shims only; these are not application changes. Full synthetic
OSM-to-MWM generation and runtime provider checks cover a winding river, an island
lake, an outlet, a disconnected lake and unsupported culverts/aqueducts.
Real Lake District runtime checks cover Easedale Tarn, its outlet to Sourmilk Gill,
Rydal Water around Heron Island and winding river segments. Source data are kept
unaltered; these checks use the optional section in a generated regional MWM.
Compatibility checks also use the unmodified official Lancaster MWM downloaded
from the Organic Maps CDN, with no water_geometry section. Generator tests create
a standard MWM containing an island, winding river, crossing and coincident river
endpoints; they cover absent coverage proof, cancellation and ignored aqueduct
restrictions. A separate packed-border regression rejects a concave coverage gap
even when all rectangle corners and its bounding box appear covered.
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
Water now delegates this initial approach to the unchanged pedestrian IndexRouter.
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
52.354819,5.179565 to 52.355862,5.185028. The unchanged pedestrian IndexRouter
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
