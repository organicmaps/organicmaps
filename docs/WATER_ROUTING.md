# Water distance measurement

Android Water measures distance along mapped water geometry. Water portions are solid;
a necessary land transfer is dashed. Results show water distance and, when present,
land distance and their sum in application units. There is no navigation, ETA, speed
or elevation chart. Ruler still measures straight segments.

Select an explicit start with Route from, choose Water in the toolbar, then select
a destination and optional intermediate points on water. An implicit GPS start
projects onto the nearest known water. Explicit selections never project across
land. River taps snap to the selected displayed centerline. Other non-water points
have no route actions. Line selections allow 0.1 m of quantization tolerance.
An explicit point outside water reports which checkpoint needs moving. Near a
shore, zoom in and select a point farther inside the stored water geometry.

## Routing and transfers

WaterRouter implements IRouter without a road vehicle profile. River lines retain
original OpenStreetMap coordinates and node IDs; crossings alone do not join them.
Surface union and triangulation preserve holes. Visibility edges require complete
water coverage. River/surface transitions use actual intersections. Distances use
mercator::DistanceOnEarth. Checkpoints retain their selected order.

Connected water paths always take precedence over land shortcuts. Otherwise each
checkpoint leg may contain one necessary straight land interval, minimizing total
leg length. Line-only transfers use original source-way endpoints. Interior
vertices, selected points and extraction cuts are not transfer points. Surface
transfers use shoreline intersections. Candidates crossing a third disconnected
water component are rejected. Only the internal arrival marker required by shared
presentation is produced. Android hides navigation controls; native following is disabled.

## Ordinary downloaded maps

Water can use an ordinary country MWM without regenerating it. The compatibility
provider reads its best available waterway lines and water-surface triangles,
unions the surfaces, and preserves the islands represented by that geometry.
Distances follow the stored display geometry, which may be simplified relative
to the original OpenStreetMap data.

Original river node IDs are absent. Separate line features never join merely
because their coordinates coincide or cross. River/surface intersections do join.
As a deliberate measurement simplification, bridge, aqueduct and lock restrictions
are ignored in this compatibility mode, including positive layers. This can join
an aqueduct to a water surface it crosses. Underground waterways remain unknown.
This mode does not model navigability, water depth, currents or access restrictions.

If no connected water path exists in the compatibility graph, a leg may include
one straight land transfer only between endpoints of stored river lines. Interior
vertices, checkpoints, extraction cuts and arbitrary lake shores cannot be transfer
points. An endpoint inside a lake cannot move the dry interval to its shoreline.
Stored feature endpoints need not be original OpenStreetMap way endpoints. Missing
junctions may therefore require a measured land transfer or make a route unavailable.
The computed minimum refers to connections represented in this graph, not
reconstructed source topology or proof of a physically necessary portage.
Coastline polygons describe land and do not supply ocean water in this mode.

The complete query rectangle must lie inside a known map-region polygon. The
application supplies this proof from packed borders, with a margin for border
simplification and quantization. A feature bounding box alone never certifies
coverage. Missing borders, cross-region queries and exceeded budgets remain
unavailable. Android point eligibility uses the same coverage check.
Eligibility uses a 50 m margin instead of extracting a full routing window
for a single point. The feature budget counts intersecting water features,
excluding buildings and roads.
Packed borders describe application map regions; they cannot reconstruct exact
original source-extract coverage from an old MWM.

## Optional MWM section

An optional water_geometry section retains original source topology and takes
precedence over the compatibility provider. It also permits certified land
transfers. Generate these maps with normal resources and borders:

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
Cross-MWM routes, antimeridian windows and
requests exceeding budgets return unavailable. A reachable
graph touching the extraction frontier cannot establish disconnection. Connected
paths are accepted only when shorter than a proven lower bound for leaving the window.
Unknown regions also require a distance bound excluding a shorter route through
them. Disconnected components touching unknown geometry cannot justify a land gap.

The reader caches one index with at most 65,536 records. Extraction allows 4,096
surface source points and 16,384 triangles, with 16,384 line source points. The graph
allows 65,536 vertices. Lazy A* starts with valid sparse mesh paths and checks
visibility only when it can improve the route. It allows at most 2,000,000 segment
checks and 8,000,000 indexed triangle visits. Interior triangle vertices keep linear mesh edges;
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

## Validation

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
geometry and requires moving the start into the canal. Long-path tests cover
adaptive border windows, lazy visibility, thin dry gaps and numerical cancellation
near zero.
The northward Almere case, 52.36468055555556,5.216444444444445 to
52.39769444444445,5.207547222222223, produces 5,311.6 m entirely on water.
A regression reduced from this map covers river/surface intersections along a
nearly parallel shore, both line and route directions, and indexed geometry.

Android app/SDK unit tests and debug builds run separately. Xcode projects include
the common implementation and tessellation dependencies. iOS UI is deferred; no
Apple-platform build has been run in this Windows environment.
