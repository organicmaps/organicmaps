#include "geometry/algorithm.hpp"
#include "geometry/mercator.hpp"

#include "indexer/edits_migration.hpp"
#include "indexer/feature.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"

namespace
{
m2::PointD CalculateCenter(vector<m2::PointD> const & geometry)
{
  ASSERT(!geometry.empty() && geometry.size() % 3 == 0,
         ("Invalid geometry should be handled in caller."));

  m2::CalculateBoundingBox doCalcBox;
  for (auto const & p : geometry)
    doCalcBox(p);

  m2::CalculatePointOnSurface doCalcCenter(doCalcBox.GetBoundingBox());
  for (auto i = 0; i < geometry.size() - 3; i += 3)
    doCalcCenter(geometry[i], geometry[i + 1], geometry[i + 2]);

  return doCalcCenter.GetCenter();
}

uint32_t GetGeometriesIntersectionCapacity(vector<m2::PointD> g1,
                                           vector<m2::PointD> g2)
{
  struct Counter
  {
    Counter & operator++() { return *this; }
    Counter & operator*() { return *this; }
    Counter & operator=(m2::PointD const &)
    {
      ++m_count;
      return *this;
    }
    uint32_t m_count = 0;
  };

  sort(begin(g1), end(g1));
  sort(begin(g2), end(g2));

  Counter counter;
  set_intersection(begin(g1), end(g1),
                   begin(g2), end(g2),
                   counter, [](m2::PointD const & p1, m2::PointD const & p2)
                   {
                     // TODO(mgsergio): Use 1e-7 everyware instead of MercatotBounds::GetCellID2PointAbsEpsilon
                     return p1 < p2 && !p1.EqualDxDy(p2, 1e-7);
                   });
  return counter.m_count;
}
}  // namespace

namespace editor
{
FeatureID MigrateNodeFeatureIndex(osm::Editor::TForEachFeaturesNearByFn & forEach,
                                  XMLFeature const & xml)
{
  unique_ptr<FeatureType> feature;
  auto count = 0;
  forEach(
      [&feature, &xml, &count](FeatureType const & ft)
      {
        if (ft.GetFeatureType() != feature::GEOM_POINT)
          return;
        // TODO(mgsergio): Check that ft and xml correspond to the same feature.
        feature = make_unique<FeatureType>(ft);
        ++count;
      },
      MercatorBounds::FromLatLon(xml.GetCenter()));
  if (!feature)
    MYTHROW(MigrationError, ("No pointed features returned."));
  if (count > 1)
  {
    LOG(LWARNING,
        (count, "features returned for point", MercatorBounds::FromLatLon(xml.GetCenter())));
  }
  return feature->GetID();
}

FeatureID MigrateWayFeatureIndex(osm::Editor::TForEachFeaturesNearByFn & forEach,
                                 XMLFeature const & xml)
{
  unique_ptr<FeatureType> feature;
  auto bestScore = 0.6;  // initial score is used as a threshold.
  auto const geometry = xml.GetGeometry();

  if (geometry.empty() || geometry.size() % 3 != 0)
    MYTHROW(MigrationError, ("Feature has invalid geometry", xml));

  auto const someFeaturePoint = CalculateCenter(geometry);
  auto count = 0;
  LOG(LDEBUG, ("SomePoint", someFeaturePoint));
  forEach(
      [&feature, &xml, &geometry, &count, &bestScore](FeatureType const & ft)
      {
        if (ft.GetFeatureType() != feature::GEOM_AREA)
          return;
        ++count;
        auto const ftGeometry = ft.GetTriangesAsPoints(FeatureType::BEST_GEOMETRY);
        auto matched = GetGeometriesIntersectionCapacity(ftGeometry, geometry);
        auto const score = static_cast<double>(matched) / geometry.size();
        if (score > bestScore)
        {
          bestScore = score;
          feature = make_unique<FeatureType>(ft);
        }
      },
      someFeaturePoint);
  if (count == 0)
    MYTHROW(MigrationError, ("No ways returned for point", someFeaturePoint));
  if (!feature)
  {
    MYTHROW(MigrationError,
            ("None of returned ways suffice. Possibly, the feature have been deleted."));
  }
  return feature->GetID();
}

FeatureID MigrateFeatureIndex(osm::Editor::TForEachFeaturesNearByFn & forEach,
                              XMLFeature const & xml)
{
  switch (xml.GetType())
  {
  case XMLFeature::Type::Node: return MigrateNodeFeatureIndex(forEach, xml);
  case XMLFeature::Type::Way: return MigrateWayFeatureIndex(forEach, xml);
  }
}
}  // namespace editor
