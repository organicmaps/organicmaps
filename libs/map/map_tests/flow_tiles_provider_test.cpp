#include "map/flow_tiles_provider.hpp"

#include "testing/testing.hpp"

namespace flow_tiles_provider_test
{
using namespace traffic;

UNIT_TEST(FlowTiles_SpeedGroupFromFlow_Buckets)
{
  // Midpoints of the speed-group buckets defined by GetSpeedGroupByPercentage.
  TEST_EQUAL(SpeedGroupFromFlow(0.0, false), SpeedGroup::G0, ());
  TEST_EQUAL(SpeedGroupFromFlow(0.04, false), SpeedGroup::G0, ());
  TEST_EQUAL(SpeedGroupFromFlow(0.12, false), SpeedGroup::G1, ());
  TEST_EQUAL(SpeedGroupFromFlow(0.25, false), SpeedGroup::G2, ());
  TEST_EQUAL(SpeedGroupFromFlow(0.45, false), SpeedGroup::G3, ());
  TEST_EQUAL(SpeedGroupFromFlow(0.70, false), SpeedGroup::G4, ());
  TEST_EQUAL(SpeedGroupFromFlow(0.95, false), SpeedGroup::G5, ());

  // Free flow above the nominal range is clamped to the best bucket.
  TEST_EQUAL(SpeedGroupFromFlow(1.0, false), SpeedGroup::G5, ());
  TEST_EQUAL(SpeedGroupFromFlow(1.5, false), SpeedGroup::G5, ());
}

UNIT_TEST(FlowTiles_SpeedGroupFromFlow_Specials)
{
  // Closures win over any reported speed.
  TEST_EQUAL(SpeedGroupFromFlow(1.0, true), SpeedGroup::TempBlock, ());
  TEST_EQUAL(SpeedGroupFromFlow(0.0, true), SpeedGroup::TempBlock, ());

  // Negative speeds are corrupt and map to Unknown.
  TEST_EQUAL(SpeedGroupFromFlow(-1.0, false), SpeedGroup::Unknown, ());
}

UNIT_TEST(FlowTiles_AddToColoring_ClosurePriority)
{
  using Id = TrafficInfo::RoadSegmentId;

  TrafficInfo::Coloring coloring;
  AddToColoring(coloring, Id(1 /* fid */, 0 /* idx */, Id::kForwardDirection), SpeedGroup::G3);
  TEST_EQUAL(coloring.size(), 1, ());
  TEST_EQUAL(coloring[Id(1, 0, Id::kForwardDirection)], SpeedGroup::G3, ());

  // A repeated report keeps the first value.
  AddToColoring(coloring, Id(1, 0, Id::kForwardDirection), SpeedGroup::G0);
  TEST_EQUAL(coloring.size(), 1, ());
  TEST_EQUAL(coloring[Id(1, 0, Id::kForwardDirection)], SpeedGroup::G3, ());

  // A closure from a neighboring tile overrides the stored speed ...
  AddToColoring(coloring, Id(1, 0, Id::kForwardDirection), SpeedGroup::TempBlock);
  TEST_EQUAL(coloring[Id(1, 0, Id::kForwardDirection)], SpeedGroup::TempBlock, ());

  // ... but a later speed never downgrades an existing closure.
  AddToColoring(coloring, Id(1, 0, Id::kForwardDirection), SpeedGroup::G5);
  TEST_EQUAL(coloring.size(), 1, ());
  TEST_EQUAL(coloring[Id(1, 0, Id::kForwardDirection)], SpeedGroup::TempBlock, ());
}

UNIT_TEST(FlowTiles_MergeColoring)
{
  using Id = TrafficInfo::RoadSegmentId;

  TrafficInfo::Coloring dst;
  AddToColoring(dst, Id(1, 0, Id::kForwardDirection), SpeedGroup::G2);

  TrafficInfo::Coloring src;
  AddToColoring(src, Id(1, 1, Id::kForwardDirection), SpeedGroup::G4);
  AddToColoring(src, Id(2, 0, Id::kReverseDirection), SpeedGroup::TempBlock);

  MergeColoring(dst, src);
  TEST_EQUAL(dst.size(), 3, ());
  TEST_EQUAL(dst[Id(1, 0, Id::kForwardDirection)], SpeedGroup::G2, ());
  TEST_EQUAL(dst[Id(1, 1, Id::kForwardDirection)], SpeedGroup::G4, ());
  TEST_EQUAL(dst[Id(2, 0, Id::kReverseDirection)], SpeedGroup::TempBlock, ());

  // Merging a tile that reports a closure on an already-known segment wins
  // over the previously merged speed.
  TrafficInfo::Coloring closureTile;
  AddToColoring(closureTile, Id(1, 0, Id::kForwardDirection), SpeedGroup::TempBlock);
  MergeColoring(dst, closureTile);
  TEST_EQUAL(dst.size(), 3, ());
  TEST_EQUAL(dst[Id(1, 0, Id::kForwardDirection)], SpeedGroup::TempBlock, ());
}
}  // namespace flow_tiles_provider_test
