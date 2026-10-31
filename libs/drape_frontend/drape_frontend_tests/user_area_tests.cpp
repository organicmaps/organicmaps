#include "testing/testing.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/user_mark_generator.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/utils/projection.hpp"

#include "kml/area_data.hpp"
#include "kml/serdes.hpp"

#include "platform/platform.hpp"

#include "coding/zip_reader.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>

namespace user_area_tests
{
kml::AreaRing Ring(std::initializer_list<m2::PointD> lonLats)
{
  kml::AreaRing ring;
  for (auto const & p : lonLats)
    ring.push_back({{p.y, p.x}, {}});
  return ring;
}

// A 10x10 degrees square with a 6x6 degrees hole.
kml::AreaData MakeArea()
{
  kml::AreaData area;
  area.m_style.m_fillColor = 0xFF000080;
  area.m_polygons.push_back({Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}}), {Ring({{2, 2}, {8, 2}, {8, 8}, {2, 8}})}});
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  return area;
}

double const kSquareWithHoleArea = 10 * mercator::LatToY(10) - 6 * (mercator::LatToY(8) - mercator::LatToY(2));

drape_ptr<df::UserAreaRenderParams> Tessellate(kml::AreaData const & area)
{
  auto info = df::GenerateAreaRenderInfo(area);
  for (size_t i = 0; i < info->m_triangles.size(); ++i)
    df::TessellateUserArea(*info, i);
  return info;
}

std::vector<m2::PointD> Triangles(df::UserAreaRenderParams const & area)
{
  std::vector<m2::PointD> triangles;
  for (auto const & t : area.m_triangles)
    triangles.insert(triangles.end(), t.begin(), t.end());
  return triangles;
}

// Checks that the triangles are clockwise, as ClipTriangleByRect requires, and returns their area.
double ClockwiseArea(std::vector<m2::PointD> const & t)
{
  TEST_EQUAL(t.size() % 3, 0, ());
  double area = 0;
  for (size_t i = 0; i < t.size(); i += 3)
  {
    double const s = m2::robust::OrientedS(t[i], t[i + 1], t[i + 2]);
    TEST_LESS(s, 0, (t[i], t[i + 1], t[i + 2]));
    area -= s / 2;
  }
  return area;
}

m2::PointD LonLat(double lon, double lat)
{
  return mercator::FromLatLon(lat, lon);
}

// The number of clockwise triangles that cover a point.
int Coverage(std::vector<m2::PointD> const & t, m2::PointD const & p)
{
  int count = 0;
  for (size_t i = 0; i < t.size(); i += 3)
  {
    count += m2::robust::OrientedS(t[i], t[i + 1], p) < 0 && m2::robust::OrientedS(t[i + 1], t[i + 2], p) < 0 &&
             m2::robust::OrientedS(t[i + 2], t[i], p) < 0;
  }
  return count;
}

// An oracle independent of tessellation: polygons cover a point by the even-odd rule over their rings.
int EvenOddCoverage(kml::AreaData const & area, m2::PointD const & p)
{
  int count = 0;
  for (auto const & polygon : area.m_polygons)
  {
    bool inside = false;
    auto const cross = [&](kml::AreaRing const & ring)
    {
      for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
      {
        auto const a = mercator::FromLatLon(ring[j].m_latLon);
        auto const b = mercator::FromLatLon(ring[i].m_latLon);
        if ((a.y > p.y) != (b.y > p.y) && (m2::robust::OrientedS(a, b, p) > 0) == (b.y > a.y))
          inside = !inside;
      }
    };
    cross(polygon.m_outer);
    for (auto const & hole : polygon.m_holes)
      cross(hole);
    count += inside;
  }
  return count;
}

void CheckCoverage(kml::AreaData const & area, df::UserAreaRenderParams const & info)
{
  // Offsets keep the samples off shared edges.
  auto const triangles = Triangles(info);
  auto const & r = info.m_limitRect;
  for (int y = 0; y < 8; ++y)
  {
    for (int x = 0; x < 8; ++x)
    {
      m2::PointD const p(r.minX() + (x + 0.371) * r.SizeX() / 8, r.minY() + (y + 0.631) * r.SizeY() / 8);
      TEST_EQUAL(Coverage(triangles, p), EvenOddCoverage(area, p), (p));
    }
  }
}

kml::MarkGroupId constexpr kGroupId = 1;

// Shows the areas with IDs 1, 2, ... in one group.
template <class MakeInfo>
void ShowAreas(df::UserMarkGenerator & generator, kml::AreaId count, MakeInfo && makeInfo)
{
  auto areas = make_unique_dp<df::UserAreasRenderCollection>();
  auto ids = make_unique_dp<df::IDCollections>();
  for (kml::AreaId id = 1; id <= count; ++id)
  {
    areas->emplace(id, makeInfo(id));
    ids->m_areaIds.push_back(id);
  }
  generator.SetUserAreas(std::move(areas));
  generator.SetGroup(kGroupId, std::move(ids));
  generator.SetGroupVisibility(kGroupId, true);
}

// One group with two areas and changes of the group.
class AreasProvider : public df::UserMarksProvider
{
public:
  kml::GroupIdSet const & GetUpdatedGroupIds() const override { return m_updatedGroups; }
  kml::GroupIdSet const & GetRemovedGroupIds() const override { return m_removedGroups; }
  kml::GroupIdSet GetAllGroupIds() const override { return {kGroupId}; }
  kml::GroupIdSet const & GetBecameVisibleGroupIds() const override { return m_becameVisibleGroups; }
  kml::GroupIdSet const & GetBecameInvisibleGroupIds() const override { return m_becameInvisibleGroups; }
  bool IsGroupVisible(kml::MarkGroupId) const override { return m_visible; }
  kml::MarkIdSet const & GetGroupPointIds(kml::MarkGroupId) const override { return m_noMarks; }
  kml::TrackIdSet const & GetGroupLineIds(kml::MarkGroupId) const override { return m_noLines; }
  kml::MarkIdSet const & GetCreatedMarkIds() const override { return m_noMarks; }
  kml::MarkIdSet const & GetRemovedMarkIds() const override { return m_noMarks; }
  kml::MarkIdSet const & GetUpdatedMarkIds() const override { return m_noMarks; }
  kml::TrackIdSet const & GetCreatedLineIds() const override { return m_noLines; }
  kml::TrackIdSet const & GetRemovedLineIds() const override { return m_noLines; }
  kml::TrackIdSet const & GetUpdatedLineIds() const override { return m_noLines; }
  df::UserPointMark const * GetUserPointMark(kml::MarkId) const override { return nullptr; }
  df::UserLineMark const * GetUserLineMark(kml::TrackId) const override { return nullptr; }
  kml::AreaIdSet const & GetGroupAreaIds(kml::MarkGroupId) const override { return m_groupAreas; }
  kml::AreaIdSet const & GetCreatedAreaIds() const override { return m_noAreas; }
  kml::AreaIdSet const & GetRemovedAreaIds() const override { return m_removedAreas; }
  kml::AreaData const * GetUserArea(kml::AreaId) const override { return &m_area; }

  bool m_visible = true;
  kml::GroupIdSet m_updatedGroups, m_removedGroups, m_becameVisibleGroups, m_becameInvisibleGroups;
  kml::AreaIdSet m_removedAreas;

private:
  kml::AreaIdSet m_groupAreas = {1, 2}, m_noAreas;
  kml::MarkIdSet m_noMarks;
  kml::TrackIdSet m_noLines;
  kml::AreaData m_area = MakeArea();
};

std::vector<kml::AreaId> SentAreaIds(df::DrapeEngine::UserMarksUpdate const & update)
{
  std::vector<kml::AreaId> ids;
  for (auto const & [id, info] : *update.m_areas)
    ids.push_back(id);
  std::ranges::sort(ids);
  return ids;
}

UNIT_TEST(UserAreas_RenderInfo)
{
  auto area = MakeArea();
  area.m_style.m_outlineColor = 0x0000FFFF;
  area.m_style.m_outlineWidth = 3;
  auto info = df::GenerateAreaRenderInfo(area);
  TEST_EQUAL(info->m_fillColor, dp::Color(0xFF000080), ());
  TEST_EQUAL(info->m_outlineColor, dp::Color(0x0000FFFF), ());
  TEST_EQUAL(info->m_outlineWidth, 3, ());
  TEST_EQUAL(info->m_limitRect, mercator::FromLatLon(m2::RectD(0, 0, 10, 10)), ());
  TEST_EQUAL(info->m_polygons.size(), 1, ());
  TEST_EQUAL(info->m_polygons.front().size(), 2, ("The outer ring and the hole"));
  for (auto const & ring : info->m_polygons.front())
  {
    // 4 corners, and both ends in the middle of the closing edge.
    auto const & path = ring->GetPath();
    TEST_EQUAL(path.size(), 6, ());
    TEST_EQUAL(path.front(), path.back(), ());
    TEST_EQUAL(path.front(), (path[1] + path[4]) / 2, ());
  }

  area.m_style.m_fill = false;
  area.m_style.m_outlineColor = 0x0000FF00;
  info = Tessellate(area);
  TEST(info->m_triangles.empty(), ("No fill"));
  TEST_EQUAL(info->m_outlineWidth, 0, ("Transparent outline"));

  area.m_visible = false;
  info = df::GenerateAreaRenderInfo(area);
  TEST(info->m_polygons.empty() && info->m_limitRect.IsEmptyInterior(), ("Hidden area"));
}

UNIT_TEST(UserAreas_HeightOnlyVerticesKeepValidFootprint)
{
  kml::AreaData area;
  area.m_polygons.push_back({Ring({{0, 0}, {2, 0}, {2, 0}, {0, 2}}), {}});
  auto & ring = area.m_polygons.front().m_outer;
  ring[1].m_altitude = 5;
  ring[2].m_altitude = 8;
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  TEST_EQUAL(ring.size(), 4, ("Altitude fidelity retains both source vertices"));
  auto const info = Tessellate(area);
  TEST_EQUAL(info->m_polygons.front().front()->GetSize(), 5, ("Splines need nonzero segments"));
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), mercator::LatToY(2), 1e-8, ());
}

UNIT_TEST(UserAreas_GlobalAndPolarFootprintsAreSkipped)
{
  for (auto const & outer :
       {Ring({{-100, 0}, {100, 0}, {100, 10}, {-100, 10}}), Ring({{0, 80}, {10, 80}, {10, 89}, {0, 89}})})
  {
    kml::AreaData area;
    area.m_polygons.push_back({outer, {}});
    auto const info = Tessellate(area);
    TEST(info->m_polygons.empty(), ());
    TEST(info->m_triangles.empty(), ());
    TEST(info->m_limitRect.IsEmptyInterior(), ());
  }

  // Parts of an area are checked separately, and neither of these near the antimeridian needs wrapping.
  kml::AreaData area;
  area.m_polygons = {{Ring({{-179, 0}, {-178, 0}, {-178, 1}, {-179, 1}}), {}},
                     {Ring({{178, 0}, {179, 0}, {179, 1}, {178, 1}}), {}},
                     {Ring({{0, 80}, {10, 80}, {10, 89}, {0, 89}}), {}}};
  auto const info = Tessellate(area);
  TEST_EQUAL(info->m_polygons.size(), 2, ());
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), 2 * mercator::LatToY(1), 1e-8, ());
  TEST(!info->m_polygonRects.front().IsIntersect(info->m_polygonRects.back()), ());
}

UNIT_TEST(UserAreas_TessellationKeepsHolesAndOverlaps)
{
  auto area = MakeArea();
  auto info = Tessellate(area);
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), kSquareWithHoleArea, 1e-8, ());
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(1, 1)), 1, ());
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(5, 5)), 0, ("The hole"));

  // Overlapping polygons of one area are filled independently and don't cancel each other out.
  area.m_polygons.push_back(area.m_polygons.front());
  info = Tessellate(area);
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), 2 * kSquareWithHoleArea, 1e-8, ());
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(1, 1)), 2, ());
  // Each polygon keeps its triangles and bounds, so that tiles skip polygons beyond them.
  TEST_EQUAL(info->m_triangles.size(), 2, ());
  TEST_EQUAL(info->m_triangles.front(), info->m_triangles.back(), ());
  TEST_EQUAL(info->m_polygonRects.size(), 2, ());
  TEST_EQUAL(info->m_polygonRects.front(), info->m_limitRect, ());
}

UNIT_TEST(UserAreas_FillKeepsVerticesThatSplinesSkip)
{
  // A ring shorter than the minimal spline segment is filled, while its outline collapses into a point.
  kml::AreaData area;
  area.m_polygons = {{Ring({{0, 0}, {5e-8, 0}, {0, 5e-8}}), {}}};
  auto info = Tessellate(area);
  TEST_EQUAL(info->m_polygons.front().front()->GetSize(), 1, ());
  TEST_EQUAL(Triangles(*info).size(), 3, ());

  area = MakeArea();
  area.m_polygons.front().m_holes.push_back(Ring({{1, 1}, {1, 1 + 5e-8}, {1 + 5e-8, 1}}));
  info = Tessellate(area);
  TEST_EQUAL(info->m_polygons.front().back()->GetSize(), 1, ());
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(1 + 1e-8, 1 + 1e-8)), 0, ("The tiny hole"));
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(1, 2)), 1, ());

  // The first and the last vertices differ only in height, so the outline doesn't start in the middle of an edge.
  area.m_polygons = {{Ring({{1, 1}, {4, 4}, {1, 4}, {1, 1}}), {}}};
  area.m_polygons.front().m_outer.front().m_altitude = 2;
  area.m_polygons.front().m_outer.back().m_altitude = 1;
  info = Tessellate(area);
  auto const a = LonLat(1, 1), b = LonLat(4, 4), c = LonLat(1, 4);
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), std::fabs(m2::CrossProduct(b - a, c - a)) / 2, 1e-12, ());
}

UNIT_TEST(UserAreas_RemovedAreasAreSkippedUntilGroupUpdate)
{
  // Tiles may be generated between removing an area and updating its group.
  df::UserMarkGenerator generator([](df::TUserMarksRenderData &&) {});
  ShowAreas(generator, 2, [](kml::AreaId) { return df::GenerateAreaRenderInfo(MakeArea()); });
  auto const rect = mercator::Bounds::FullRect();
  TEST_EQUAL(generator.GetAreasToCache(rect).size(), 2, ());

  auto removed = make_unique_dp<df::IDCollections>();
  removed->m_areaIds = {1};
  generator.SetRemovedUserMarks(std::move(removed));
  auto const toCache = generator.GetAreasToCache(rect);
  TEST_EQUAL(toCache.size(), 1, ());
  TEST_EQUAL(toCache.front().second, 1, ());

  // A removed area is sent again when it's shown.
  auto areas = make_unique_dp<df::UserAreasRenderCollection>();
  areas->emplace(1, df::GenerateAreaRenderInfo(MakeArea()));
  generator.SetUserAreas(std::move(areas));
  auto const shown = generator.GetAreasToCache(rect);
  TEST_EQUAL(shown.size(), 2, ());

  // A full refresh resends shown areas, which keep their meshes, made when a tile needs them.
  TEST(shown.back().first->m_triangles.front().empty(), ());
  df::TessellateUserArea(*shown.back().first, 0);
  areas = make_unique_dp<df::UserAreasRenderCollection>();
  areas->emplace(2, df::GenerateAreaRenderInfo(MakeArea()));
  generator.SetUserAreas(std::move(areas));
  auto const refreshed = generator.GetAreasToCache(rect);
  TEST_EQUAL(refreshed.size(), 2, ());
  TEST_EQUAL(refreshed.back().first, shown.back().first, ());
  TEST(!refreshed.back().first->m_triangles.front().empty(), ());
}

UNIT_TEST(UserAreas_UpdatesRemoveAndResendAreas)
{
  // A deleted category isn't updated, but its areas are removed.
  AreasProvider deleted;
  deleted.m_removedGroups = {kGroupId};
  deleted.m_removedAreas = {1, 2};
  auto update = df::DrapeEngine::CollectUserMarksUpdate(&deleted, false /* firstTime */, settings::Placement::None);
  TEST_EQUAL(update.m_removedIds->m_areaIds, kml::AreaIdCollection({1, 2}), ());
  TEST(update.m_areas->empty() && update.m_groups.empty(), ());

  // A hidden category removes its areas and sends them again when it's shown.
  AreasProvider hidden;
  hidden.m_updatedGroups = hidden.m_becameInvisibleGroups = {kGroupId};
  hidden.m_visible = false;
  update = df::DrapeEngine::CollectUserMarksUpdate(&hidden, false /* firstTime */, settings::Placement::None);
  TEST_EQUAL(update.m_removedIds->m_areaIds, kml::AreaIdCollection({1, 2}), ());
  TEST(update.m_groups.at(kGroupId)->m_areaIds.empty(), ());

  AreasProvider shown;
  shown.m_updatedGroups = shown.m_becameVisibleGroups = {kGroupId};
  update = df::DrapeEngine::CollectUserMarksUpdate(&shown, false /* firstTime */, settings::Placement::None);
  TEST_EQUAL(SentAreaIds(update), std::vector<kml::AreaId>({1, 2}), ());
  TEST_EQUAL(update.m_groups.at(kGroupId)->m_areaIds, kml::AreaIdCollection({1, 2}), ());

  // A full refresh resends shown areas, while the backend keeps their meshes.
  update = df::DrapeEngine::CollectUserMarksUpdate(&shown, true /* firstTime */, settings::Placement::None);
  TEST_EQUAL(SentAreaIds(update), std::vector<kml::AreaId>({1, 2}), ());
}

UNIT_TEST(UserAreas_HolesOutsideShellsAreInTileBounds)
{
  kml::AreaData area;
  area.m_polygons = {{Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}}), {Ring({{12, 12}, {14, 12}, {14, 14}, {12, 14}})}}};
  df::UserMarkGenerator generator([](df::TUserMarksRenderData &&) {});
  ShowAreas(generator, 1, [&](kml::AreaId) { return df::GenerateAreaRenderInfo(area); });
  TEST_EQUAL(generator.GetAreasToCache(mercator::FromLatLon(m2::RectD(12.5, 12.5, 13.5, 13.5))).size(), 1, ());
}

UNIT_TEST(UserAreas_DepthsStayDistinctForManyAreas)
{
  // More visible areas than integers in the depth range, and only the last two are in the tile.
  auto constexpr count = static_cast<kml::AreaId>(dp::kMaxDepth) + 1;
  df::UserMarkGenerator generator([](df::TUserMarksRenderData &&) {});
  ShowAreas(generator, count, [](kml::AreaId id)
  {
    auto info = make_unique_dp<df::UserAreaRenderParams>();
    info->m_limitRect = id + 2 > count ? m2::RectD(0, 0, 1, 1) : m2::RectD(10, 10, 11, 11);
    return info;
  });
  auto const toCache = generator.GetAreasToCache({0, 0, 1, 1});
  TEST_EQUAL(toCache.size(), 2, ());
  TEST_LESS(toCache.front().second, toCache.back().second, ());
  TEST_LESS_OR_EQUAL(toCache.back().second, dp::kMaxDepth, ());
}

UNIT_TEST(UserAreas_AnyWindingAndCollinearVerticesClipByTiles)
{
  for (bool reversed : {false, true})
  {
    kml::AreaData area;
    auto & polygon = area.m_polygons.emplace_back();
    // Collinear vertices along a parallel and a meridian make zero-area triangles.
    polygon.m_outer = Ring({{0, 0}, {5, 0}, {10, 0}, {10, 5}, {10, 10}, {5, 10}, {0, 10}, {0, 5}});
    polygon.m_holes = {Ring({{2, 2}, {2, 8}, {8, 8}, {8, 2}})};
    if (reversed)
    {
      std::ranges::reverse(polygon.m_outer);
      std::ranges::reverse(polygon.m_holes.front());
    }
    auto const info = Tessellate(area);
    auto const t = Triangles(*info);
    TEST_ALMOST_EQUAL_ABS(ClockwiseArea(t), kSquareWithHoleArea, 1e-8, (reversed));

    double clipped = 0;
    for (auto const & tile : {m2::RectD(-1, -1, 5, 11), m2::RectD(5, -1, 11, 11)})
    {
      std::vector<m2::PointD> inTile;
      for (size_t i = 0; i < t.size(); i += 3)
      {
        m2::ClipTriangleByRect(mercator::FromLatLon(tile), t[i], t[i + 1], t[i + 2],
                               [&](auto const & a, auto const & b, auto const & c)
        { inTile.insert(inTile.end(), {a, b, c}); });
      }
      clipped += ClockwiseArea(inTile);
    }
    TEST_ALMOST_EQUAL_ABS(clipped, kSquareWithHoleArea, 1e-8, (reversed));
  }
}

UNIT_TEST(UserAreas_CollinearVerticesAreRemoved)
{
  // Otherwise earcut is quadratic on the densified square.
  kml::AreaData area;
  auto & outer = area.m_polygons.emplace_back().m_outer;
  for (auto const & [from, to] : std::initializer_list<std::pair<m2::PointD, m2::PointD>>{
           {{0, 0}, {10, 0}}, {{10, 0}, {10, 10}}, {{10, 10}, {0, 10}}, {{0, 10}, {0, 0}}})
  {
    for (int i = 0; i < 5000; ++i)
    {
      auto const p = from + (to - from) * (i / 5000.0);
      outer.push_back({{p.y, p.x}, {}});
    }
  }
  auto info = Tessellate(area);
  TEST_EQUAL(Triangles(*info).size(), 6, ());
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), 10 * mercator::LatToY(10), 1e-8, ());
}

UNIT_TEST(UserAreas_InvalidPolygonsAreFilledByEvenOddRule)
{
  // Earcut misfills invalid polygons, so libtess2 fills them.
  kml::AreaData area;
  area.m_polygons = {{Ring({{0, 0}, {4, 4}, {0, 4}, {4, 0}}), {}}};
  auto info = Tessellate(area);
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), 2 * mercator::LatToY(4), 1e-8, ());
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(2, 3)), 1, ("A lobe"));
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(1, 2)), 0, ("Beside the crossing"));

  area.m_polygons = {{Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}}), {Ring({{12, 12}, {14, 12}, {14, 14}, {12, 14}})}}};
  info = Tessellate(area);
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)),
                        10 * mercator::LatToY(10) + 2 * (mercator::LatToY(14) - mercator::LatToY(12)), 1e-8, ());
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(13, 13)), 1, ("The hole outside the shell"));
  TEST_EQUAL(Coverage(Triangles(*info), LonLat(11, 11)), 0, ());

  // A ring traced twice has the area of two squares, while it covers each point twice, so it's empty.
  auto const square = Ring({{0, 0}, {10, 0}, {10, 10}, {0, 10}});
  auto twice = square;
  twice.insert(twice.end(), square.begin(), square.end());
  area.m_polygons = {{twice, {}}};
  TEST_EQUAL(Coverage(Triangles(*Tessellate(area)), LonLat(5, 5)), 0, ());
}

UNIT_TEST(UserAreas_StarShapedRingsAreFans)
{
  // A jagged circle, on which earcut is slow.
  kml::AreaData area;
  auto & outer = area.m_polygons.emplace_back().m_outer;
  int constexpr kCount = 1000;
  for (int i = 0; i < kCount; ++i)
  {
    double const angle = 2 * math::pi * i / kCount;
    double const radius = i % 2 == 0 ? 40 : 50;
    outer.push_back({mercator::ToLatLon({50 + radius * std::cos(angle), 50 + radius * std::sin(angle)}), {}});
  }
  auto const info = Tessellate(area);
  TEST_EQUAL(Triangles(*info).size(), 3 * kCount, ());
  CheckCoverage(area, *info);

  // A pentagram, which winds around the center twice, and a bowtie aren't fans.
  for (auto const & ring : {Ring({{0, 3}, {2, -3}, {-3, 1}, {3, 1}, {-2, -3}}), Ring({{0, 0}, {4, 4}, {0, 4}, {4, 0}})})
  {
    area.m_polygons = {{ring, {}}};
    CheckCoverage(area, *Tessellate(area));
  }
}

UNIT_TEST(UserAreas_IntersectionInterpolationPreservesCoverage)
{
  // libtess2 misfills these crossing rings with fast math or contracted multiply-adds.
  auto const ring = [](std::initializer_list<m2::PointD> points)
  {
    kml::AreaRing result;
    for (auto const & p : points)
      result.push_back({mercator::ToLatLon(p), {}});
    return result;
  };
  kml::AreaData area;
  area.m_polygons = {{ring({{25, 24},
                            {35, 61},
                            {7, 53},
                            {61, 52},
                            {64, 55},
                            {91, 18},
                            {53, 73},
                            {3, 54},
                            {80, 38},
                            {54, 15},
                            {1, 98},
                            {27, 26},
                            {92, 28},
                            {39, 27},
                            {64, 2},
                            {16, 50}}),
                      {ring({{18, 80},
                             {13, 26},
                             {14, 75},
                             {74, 77},
                             {88, 92},
                             {36, 65},
                             {42, 98},
                             {63, 16},
                             {86, 92},
                             {3, 23},
                             {72, 75},
                             {2, 49},
                             {25, 28},
                             {99, 85},
                             {98, 10}})}}};
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  CheckCoverage(area, *Tessellate(area));

  area.m_polygons = {{ring({{5, 55}, {8, 29}, {66, 39}, {21, 32}, {35, 34}, {17, 7}, {72, 82}}),
                      {ring({{5, 61}, {5, 94}, {98, 28}, {61, 66}, {25, 97}, {98, 6}}), ring({{41, 7},
                                                                                              {21, 91},
                                                                                              {58, 18},
                                                                                              {46, 24},
                                                                                              {72, 0},
                                                                                              {63, 37},
                                                                                              {76, 4},
                                                                                              {36, 31},
                                                                                              {97, 93},
                                                                                              {36, 28},
                                                                                              {78, 43},
                                                                                              {35, 60}})}}};
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  CheckCoverage(area, *Tessellate(area));
}

UNIT_TEST(UserAreas_ManyHolesAreFilledByEvenOddRule)
{
  // Too many holes for earcut.
  auto const square = [](double x, double y, double size)
  {
    kml::AreaRing ring;
    for (auto const & p :
         {m2::PointD(x, y), m2::PointD(x + size, y), m2::PointD(x + size, y + size), m2::PointD(x, y + size)})
    {
      ring.push_back({mercator::ToLatLon(p), {}});
    }
    return ring;
  };
  kml::AreaData area;
  auto & polygon = area.m_polygons.emplace_back();
  polygon.m_outer = square(0, 0, 100);
  for (int y = 0; y < 16; ++y)
    for (int x = 0; x < 16; ++x)
      polygon.m_holes.push_back(square(2 + 6 * x, 2 + 6 * y, 1));
  // Coincident rings cancel each other out.
  polygon.m_holes.push_back(polygon.m_holes.front());
  TEST(kml::NormalizeAreaGeometry(area.m_polygons), ());
  auto const info = Tessellate(area);
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Triangles(*info)), 10000.0 - 255.0, 1e-8, ());
  CheckCoverage(area, *info);
}

UNIT_TEST(UserAreas_RealLifeTessellation)
{
  // Dartmoor camping areas, one with a hole that touches itself at a vertex.
  kml::FileData data;
  auto const kmz = GetPlatform().TestsDataPathForFile("test_data/kml/dartmoor_backpack_camping.kmz");
  kml::DeserializerKml(data).Deserialize(*ZipFileReader::CreateModelReader(kmz, "doc.kml"));
  TEST_EQUAL(data.m_areasData.size(), 70, ());
  for (auto const & area : data.m_areasData)
  {
    auto const info = Tessellate(area);
    auto const triangles = Triangles(*info);
    TEST_GREATER(ClockwiseArea(triangles), 0, (area.m_id));
    auto const & r = info->m_limitRect;
    for (double const x : {0.13, 0.31, 0.52, 0.71, 0.89})
    {
      for (double const y : {0.11, 0.33, 0.49, 0.68, 0.87})
      {
        m2::PointD const p(r.minX() + r.SizeX() * x, r.minY() + r.SizeY() * y);
        TEST_EQUAL(Coverage(triangles, p), EvenOddCoverage(area, p), (area.m_id, p));
      }
    }
  }
}
}  // namespace user_area_tests
