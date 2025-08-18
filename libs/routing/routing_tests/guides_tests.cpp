#include "testing/testing.hpp"

#include "routing/guides_connections.hpp"

#include "geometry/mercator.hpp"

namespace
{
using namespace routing;

//  10 guide, track 0        10 guide, track 1     11 guide, track 0
//  4 points                 3 points              3 points
//
//                                    X  2         X 2
//                                   X            X
//                                   X           X
//                                  X           X
//      2  XXXXX 3                  X          X 1
//        X                        X           X
//       X                         X           X
//      X                         X            X
//     X                          X 1          X
//  XXX 1                       XX             X 0
//  0                         XX
//                          XX
//                        XX 0

GuidesTracks GetTestGuides()
{
  // Two guides. First with 2 tracks, second - with 1 track.
  GuidesTracks guides;
  guides[10] = {{{mercator::FromLatLon(48.13999, 11.56873), 5},
                 {mercator::FromLatLon(48.14096, 11.57246), 5},
                 {mercator::FromLatLon(48.14487, 11.57259), 6}},
                {{mercator::FromLatLon(48.14349, 11.56712), 8},
                 {mercator::FromLatLon(48.14298, 11.56604), 6},
                 {mercator::FromLatLon(48.14223, 11.56362), 6},
                 {mercator::FromLatLon(48.14202, 11.56283), 6}}};
  guides[11] = {{{mercator::FromLatLon(48.14246, 11.57814), 9},
                 {mercator::FromLatLon(48.14279, 11.57941), 10},
                 {mercator::FromLatLon(48.14311, 11.58063), 10}}};
  return guides;
}

UNIT_TEST(Guides_SafeInit)
{
  GuidesConnections guides;
  TEST(!guides.IsActive(), ());
  TEST(!guides.IsAttached(), ());
  TEST(!guides.IsCheckpointAttached(0 /* checkpointIdx */), ());
}

UNIT_TEST(Guides_InitWithGuides)
{
  GuidesConnections guides(GetTestGuides());
  TEST(guides.IsActive(), ());
  TEST(!guides.IsAttached(), ());
}

UNIT_TEST(Guides_TooFarCheckpointsAreNotAttached)
{
  GuidesConnections guides(GetTestGuides());
  TEST(guides.IsActive(), ());
  TEST(!guides.IsAttached(), ());

  // Checkpoints located further from guides than |kMaxDistToTrackPointM|.
  std::vector<m2::PointD> const & checkpoints{mercator::FromLatLon(48.13902, 11.57113),
                                              mercator::FromLatLon(48.14589, 11.56911)};

  guides.ConnectToGuidesGraph(checkpoints);
  TEST(!guides.IsAttached(), ());

  for (size_t checkpointIdx = 0; checkpointIdx < checkpoints.size(); ++checkpointIdx)
  {
    TEST(!guides.IsCheckpointAttached(checkpointIdx), ());
    TEST(guides.GetOsmConnections(checkpointIdx).empty(), ());
  }
}

UNIT_TEST(Guides_FinishAndStartAttached)
{
  GuidesConnections guides(GetTestGuides());

  // Start checkpoint should be with fake ending to OSM and finish - with fake ending to the guide
  // segment.
  std::vector<m2::PointD> const & checkpoints{mercator::FromLatLon(48.13998, 11.56982),
                                              mercator::FromLatLon(48.14448, 11.57259)};

  guides.ConnectToGuidesGraph(checkpoints);
  TEST(guides.IsAttached(), ());

  for (size_t checkpointIdx = 0; checkpointIdx < checkpoints.size(); ++checkpointIdx)
  {
    TEST(guides.IsCheckpointAttached(checkpointIdx), ());
    auto const links = guides.GetOsmConnections(checkpointIdx);
    TEST(!links.empty(), ());

    bool const isCheckpointNear = guides.FitsForDirectLinkToGuide(checkpointIdx, checkpoints.size());
    auto const ending = guides.GetFakeEnding(checkpointIdx);

    // Initial projection to guides track.
    TEST_EQUAL(ending.m_projections.size(), 1, ());

    // Start point is on the track, finish point is on some distance.
    bool const isStart = checkpointIdx == 0;
    if (isStart)
      TEST(!isCheckpointNear, ());
    else
      TEST(isCheckpointNear, ());
  }
}

UNIT_TEST(Guides_NotAttachIntermediatePoint)
{
  GuidesConnections guides(GetTestGuides());

  // Intermediate checkpoints should not be attached to the guides if they are far enough even
  // if their neighbours are attached.
  std::vector<m2::PointD> const & checkpoints{mercator::FromLatLon(48.14349, 11.56712),
                                              mercator::FromLatLon(48.14493, 11.56820),
                                              mercator::FromLatLon(48.14311, 11.58063)};

  guides.ConnectToGuidesGraph(checkpoints);
  TEST(guides.IsAttached(), ());
  for (size_t checkpointIdx = 0; checkpointIdx < checkpoints.size(); ++checkpointIdx)
  {
    bool const isAttached = guides.IsCheckpointAttached(checkpointIdx);
    bool const isIntermediate = checkpointIdx == 1;
    if (isIntermediate)
      TEST(!isAttached, ());
    else
      TEST(isAttached, ());
  }
}
}  // namespace
