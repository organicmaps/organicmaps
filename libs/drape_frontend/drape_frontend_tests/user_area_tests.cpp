#include "testing/testing.hpp"

#include "drape_frontend/user_mark_generator.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "kml/area_data.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"

#include <algorithm>

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
  df::TessellateUserArea(*info);
  return info;
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
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(info->m_triangles), mercator::LatToY(2), 1e-8, ());
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
}

UNIT_TEST(UserAreas_TessellationKeepsHolesAndOverlaps)
{
  auto area = MakeArea();
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Tessellate(area)->m_triangles), kSquareWithHoleArea, 1e-8, ());

  // Overlapping polygons of one area are filled independently and don't cancel each other out.
  area.m_polygons.push_back(area.m_polygons.front());
  TEST_ALMOST_EQUAL_ABS(ClockwiseArea(Tessellate(area)->m_triangles), 2 * kSquareWithHoleArea, 1e-8, ());
}

UNIT_TEST(UserAreas_RemovedAreasAreSkippedUntilGroupUpdate)
{
  // Tiles may be generated between removing an area and updating its group.
  kml::MarkGroupId constexpr kGroupId = 1;
  df::UserMarkGenerator generator([](df::TUserMarksRenderData &&) {});
  auto areas = make_unique_dp<df::UserAreasRenderCollection>();
  for (kml::AreaId const id : {1, 2})
    areas->emplace(id, df::GenerateAreaRenderInfo(MakeArea()));
  generator.SetUserAreas(std::move(areas));
  auto ids = make_unique_dp<df::IDCollections>();
  ids->m_areaIds = {1, 2};
  generator.SetGroup(kGroupId, std::move(ids));
  generator.SetGroupVisibility(kGroupId, true);
  auto const rect = mercator::Bounds::FullRect();
  TEST_EQUAL(generator.GetAreasToCache(rect).size(), 2, ());

  auto removed = make_unique_dp<df::IDCollections>();
  removed->m_areaIds = {1};
  generator.SetRemovedUserMarks(std::move(removed));
  auto const toCache = generator.GetAreasToCache(rect);
  TEST_EQUAL(toCache.size(), 1, ());
  TEST_EQUAL(toCache.front().second, 1, ());

  // A removed area is sent again when it's shown.
  areas = make_unique_dp<df::UserAreasRenderCollection>();
  areas->emplace(1, df::GenerateAreaRenderInfo(MakeArea()));
  generator.SetUserAreas(std::move(areas));
  TEST_EQUAL(generator.GetAreasToCache(rect).size(), 2, ());
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
    auto const & t = info->m_triangles;
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
}  // namespace user_area_tests
