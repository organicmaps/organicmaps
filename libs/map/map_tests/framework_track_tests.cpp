#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/track_mark.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "geometry/mercator.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/files_container.hpp"
#include "coding/varint.hpp"

#include "defines.hpp"

#include <array>
#include <initializer_list>

namespace framework_track_tests
{
using df::test_support::VisualParamsFixture;

namespace
{
class RelationTrackFixture
  : public generator::tests_support::TestWithCustomMwms
  , public VisualParamsFixture
{
public:
  RelationTrackFixture()
  {
    using generator::tests_support::TestRoad;
    std::array<TestRoad, 4> const roads = {TestRoad({{19.0, 0.0}, {19.000016, 0.0}}, "outbound", "en"),
                                           TestRoad({{19.000016, 0.0}, {19.000005, 0.0}}, "return", "en"),
                                           TestRoad({{18.999, 0.0}, {18.998, 0.0}}, "western", "en"),
                                           TestRoad({{19.000005, 0.0}, {19.001, 0.0}}, "continuation", "en")};
    BuildCountry("RelationTracks", [&](generator::tests_support::TestMwmBuilder & builder)
    {
      for (auto const & road : roads)
        builder.Add(road);
      builder.Finish();

      auto const path = GetPlatform().WritablePathForFile("RelationTracks.mwm");
      std::array<uint32_t, 4> ids{};
      size_t found = 0;
      {
        FeaturesVectorTest features(path);
        features.GetVector().ForEach([&](FeatureType & ft, uint32_t index)
        {
          for (size_t i = 0; i < roads.size(); ++i)
            if (roads[i].Matches(ft))
            {
              ids[i] = index;
              ++found;
            }
        });
      }
      TEST_EQUAL(found, roads.size(), ());

      struct HikingRelation : feature::RouteRelationBase
      {
        HikingRelation() { m_type = Type::Hiking; }
      } relation;
      feature::FeaturesOffsetsTable::Builder offsets;
      FilesContainerW container(path, FileWriter::OP_WRITE_EXISTING);
      {
        auto writer = container.GetWriter(RELATIONS_FILE_TAG);
        auto const start = writer->Pos();
        for (auto const & members : {std::vector{ids[0], ids[1], ids[2]}, std::vector{ids[1]},
                                     std::vector{ids[1], ids[2]}, std::vector{ids[1], ids[3]}})
        {
          offsets.PushOffset(base::asserted_cast<uint32_t>(writer->Pos() - start));
          relation.Write(*writer);
          WriteVarUint(*writer, static_cast<uint32_t>(members.size()));
          int32_t previous = 0;
          for (auto const id : members)
          {
            WriteVarInt(*writer, static_cast<int32_t>(id) - previous);
            previous = id;
          }
          WriteToSink(*writer, uint8_t{0});  // No child or parent relation references.
        }
      }
      platform::tests_support::ScopedFile table("RelationTracks.rel_offs",
                                                platform::tests_support::ScopedFile::Mode::DoNotCreate);
      feature::FeaturesOffsetsTable::Build(offsets)->Save(table.GetFullPath());
      container.Write(table.GetFullPath(), RELATION_OFFSETS_FILE_TAG);
    });
  }
};

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

UNIT_CLASS_TEST(RelationTrackFixture, Framework_RelationTrackFiltersFinalGeometry)
{
  Framework framework({}, false /* loadMaps */);
  auto const seedId = CreateTrack(framework, {{{19.0, 0.0}, {19.001, 0.0}}});
  auto & bookmarks = framework.GetBookmarkManager();
  auto const [mwmId, result] = framework.RegisterMap(m_files.front());
  TEST(result == MwmSet::RegResult::Success, ());
  for (uint32_t const index : {0, 1, 2, 3})
  {
    // The public selector offers a choice between the displayed track and the relation.
    framework.ShowTrack(seedId);
    Track::TrackSelectionInfo selection;
    selection.m_trackId = kml::kTempRelationTrackId;
    selection.m_trackPoint = {19.0, 0.0};
    selection.m_relationId = {mwmId, index};
    framework.GetCurrentPlacePageInfo().SetTrackCandidates(
        {selection, Track::TrackSelectionInfo(seedId, {19.0, 0.0}, 0.0)});
    framework.SelectTrackCandidate(kml::kTempRelationTrackId, selection.m_relationId);

    auto const * track = bookmarks.GetTrack(kml::kTempRelationTrackId);
    if (index == 1)
    {
      TEST(!track, ("A collapsed relation has no runtime track"));
      TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTrackId(), seedId, ());
      continue;
    }
    TEST(track, (index));
    TEST_EQUAL(framework.GetCurrentPlacePageInfo().GetTrackId(), kml::kTempRelationTrackId, (index));
    auto const & geometry = track->GetData().m_geometry;
    TEST(geometry.IsValid(), (index));
    TEST_EQUAL(geometry.m_lines.size(), 1, (index));
    if (index == 3)
      TEST_EQUAL(geometry.m_lines.front().size(), 3, ("Keep a short member joined to a usable continuation"));
    else
      TEST_LESS(geometry.m_lines.front().front().GetPoint().x, 18.9995, ("Keep the usable western chain", index));
    TEST_GREATER(track->GetLengthMeters(), 0.0, (index));
  }
}

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
