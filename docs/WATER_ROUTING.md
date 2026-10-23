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

## Optional MWM section

Existing downloaded maps lack required topology and coverage. They remain readable,
but Water returns unavailable. Generate new maps with normal resources and borders:

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

One MWM must cover the extraction rectangle. The margin follows checkpoint distance
and is bounded between 1 and 30 km. Cross-MWM routes, antimeridian windows and
requests exceeding budgets return unavailable. A reachable
graph touching the extraction frontier cannot establish disconnection. Connected
paths are accepted only when shorter than a proven lower bound for leaving the window.
Unknown regions also require a distance bound excluding a shorter route through
them. Disconnected components touching unknown geometry cannot justify a land gap.

The reader caches one index with at most 65,536 records. Extraction allows 4,096
surface source points and 4,096 triangles, with 16,384 total source points. The graph
allows 16,384 vertices; only its surface vertices enter the quadratic visibility
search, limited to 1,024 vertices. River curvature retains all original vertices.
Transfer candidates are bounded to 2,048. Cancellation is
checked during extraction and graph searches. Tessellation is bounded by its input.
Large or complex waterways may be unavailable. Country-scale performance and map
size have not been benchmarked. The synthetic integration section occupies 1,018 bytes.

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

Android app/SDK unit tests and debug builds run separately. Xcode projects include
the common implementation and tessellation dependencies. iOS UI is deferred; no
Apple-platform build has been run in this Windows environment.
