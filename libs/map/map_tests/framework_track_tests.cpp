#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/track_mark.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "geometry/mercator.hpp"

#include <initializer_list>

namespace framework_track_tests
{
using df::test_support::VisualParamsFixture;

namespace
{
kml::TrackId CreateTrack(Framework & framework, std::initializer_list<std::initializer_list<m2::PointD>> const & lines)
{
  kml::TrackData data;
  data.m_layers.emplace_back();
  for (auto const & line : lines)
  {
    kml::MultiGeometry::LineT points;
    for (auto const & point : line)
      points.emplace_back(point);
    data.m_geometry.m_lines.push_back(std::move(points));
    data.m_geometry.m_timestamps.emplace_back();
  }

  auto & bm = framework.GetBookmarkManager();
  bm.EnableTestMode(true);
  auto const category = bm.CreateBookmarkCategory("Tracks", false /* autoSave */);
  auto session = bm.GetEditSession();
  auto const id = session.CreateTrack(std::move(data))->GetId();
  session.AttachTrack(id, category);
  return id;
}

void CheckSelection(Framework const & framework, kml::TrackId id, m2::PointD const & point, double distance)
{
  auto const & info = framework.GetCurrentPlacePageInfo();
  auto const selection = framework.GetBookmarkManager().GetTrackSelectionInfo(id);
  TEST_EQUAL(info.GetTrackId(), id, ());
  TEST_EQUAL(info.GetSelectedObject(), df::SelectionShape::OBJECT_TRACK, ());
  TEST_ALMOST_EQUAL_ABS(info.GetMercator(), point, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(selection.m_trackPoint, point, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(selection.m_distFromBegM, distance, 1e-7, ());
}
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, Framework_ShowTrackSelectsStart)
{
  using Lines = std::initializer_list<std::initializer_list<m2::PointD>>;
  Lines const cases[] = {
      {{{10, 10}, {12, 10}, {14, 10}}}, {{{10, 10}, {12, 10}, {12, 12}, {10, 12}, {10, 10}}},
      {{{10, 10}, {12, 10}, {10, 10}}}, {{{10, 10}, {11, 10}}, {{20, 20}, {21, 20}}},
      {{{14, 10}, {12, 10}, {10, 10}}}, {{{10, 10}, {10, 10}, {12, 10}}},
  };
  Framework framework({}, false /* loadMaps */);
  for (auto const & lines : cases)
  {
    auto const id = CreateTrack(framework, lines);
    framework.ShowTrack(id);
    CheckSelection(framework, id, *lines.begin()->begin(), 0.0);
    framework.UpdatePlacePageInfoForCurrentSelection();
    CheckSelection(framework, id, *lines.begin()->begin(), 0.0);
  }
}

UNIT_CLASS_TEST(VisualParamsFixture, Framework_ShowTrackResetsPreviousSelection)
{
  Framework framework({}, false /* loadMaps */);
  auto const id = CreateTrack(framework, {{{10, 10}, {12, 10}, {14, 10}}});
  auto & bm = framework.GetBookmarkManager();
  framework.ShowTrack(id);
  auto const distance = mercator::DistanceOnEarth({10, 10}, {12, 10}) * 0.5;
  bm.SetElevationActivePoint(id, distance);
  framework.UpdatePlacePageInfoForCurrentSelection();
  CheckSelection(framework, id, {11, 10}, distance);

  framework.ShowTrack(id);
  CheckSelection(framework, id, {10, 10}, 0.0);
}

UNIT_CLASS_TEST(VisualParamsFixture, Framework_TrackRefreshKeepsReturnLegSelection)
{
  Framework framework({}, false /* loadMaps */);
  auto const id = CreateTrack(framework, {{{10, 10}, {12, 10}, {10, 10}}});
  auto & bm = framework.GetBookmarkManager();
  framework.ShowTrack(id);
  // This point lies on both legs; re-snapping would select the outbound leg.
  auto const distance = mercator::DistanceOnEarth({10, 10}, {12, 10}) * 1.5;
  bm.SetElevationActivePoint(id, distance);

  framework.UpdatePlacePageInfoForCurrentSelection();
  CheckSelection(framework, id, {11, 10}, distance);
}

UNIT_CLASS_TEST(VisualParamsFixture, Framework_TrackRefreshDoesNotSelectPoiForDeletedTrack)
{
  Framework framework({}, false /* loadMaps */);
  auto const id = CreateTrack(framework, {{{10, 10}, {12, 10}}});
  auto & bm = framework.GetBookmarkManager();
  framework.ShowTrack(id);
  bm.SelectionMark().SetPtOrg({20, 20});
  bm.GetEditSession().DeleteTrack(id);

  framework.UpdatePlacePageInfoForCurrentSelection();
  auto const & info = framework.GetCurrentPlacePageInfo();
  TEST_EQUAL(info.GetSelectedObject(), df::SelectionShape::OBJECT_EMPTY, ());
  TEST_EQUAL(info.GetTrackId(), kml::kInvalidTrackId, ());
  TEST_EQUAL(bm.SelectionMark().GetPivot(), m2::PointD(20, 20), ());
}

UNIT_CLASS_TEST(VisualParamsFixture, Framework_ShowTrackIgnoresOverlappingMarks)
{
  Framework framework({}, false /* loadMaps */);
  auto const otherId = CreateTrack(framework, {{{10, 10}, {10, 12}}});
  auto const id = CreateTrack(framework, {{{10, 10}, {12, 10}}});
  auto & bm = framework.GetBookmarkManager();
  framework.ShowTrack(otherId);
  kml::BookmarkData bookmark;
  bookmark.m_point = {10, 10};
  {
    auto session = bm.GetEditSession();
    session.CreateBookmark(std::move(bookmark), bm.GetTrack(id)->GetGroupId());
    for (auto const markId : bm.GetUserMarkIds(UserMark::Type::TRACK_SELECTION))
      if (bm.GetMark<TrackSelectionMark>(markId)->GetTrackId() == otherId)
        session.GetMarkForEdit<TrackSelectionMark>(markId)->SetIsVisible(true);
  }

  framework.ShowTrack(id);
  CheckSelection(framework, id, {10, 10}, 0.0);
}

UNIT_CLASS_TEST(VisualParamsFixture, Framework_ShowTrackUnhidesTrackAndCategory)
{
  Framework framework({}, false /* loadMaps */);
  auto const id = CreateTrack(framework, {{{10, 10}, {12, 10}}});
  auto & bm = framework.GetBookmarkManager();
  auto const category = bm.GetTrack(id)->GetGroupId();
  {
    auto session = bm.GetEditSession();
    session.SetIsVisible(category, false);
    session.SetTrackVisibility(id, false);
  }

  framework.ShowTrack(id);
  CheckSelection(framework, id, {10, 10}, 0.0);
  TEST(bm.IsVisible(category), ());
  TEST(bm.GetTrack(id)->IsVisible(), ());
}
}  // namespace framework_track_tests
