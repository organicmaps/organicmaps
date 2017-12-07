#include "geometry/algorithm.hpp"
#include "geometry/mercator.hpp"

#include "indexer/edits_migration.hpp"
#include "indexer/feature.hpp"

#include "base/logging.hpp"
#include "base/stl_iterator.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"

namespace editor
{
FeatureID MigrateNodeFeatureIndex(osm::Editor::ForEachFeaturesNearByFn & forEach,
                                  XMLFeature const & xml,
                                  osm::Editor::FeatureStatus const featureStatus,
                                  TGenerateIDFn const & generateID)
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

  if (!feature && featureStatus != osm::Editor::FeatureStatus::Created)
    MYTHROW(MigrationError, ("No pointed features returned."));
  if (featureStatus == osm::Editor::FeatureStatus::Created)
    return generateID();

  if (count > 1)
  {
    LOG(LWARNING,
        (count, "features returned for point", MercatorBounds::FromLatLon(xml.GetCenter())));
  }
  return feature->GetID();
}

FeatureID MigrateWayFeatureIndex(
    osm::Editor::ForEachFeaturesNearByFn & forEach, XMLFeature const & xml,
    osm::Editor::FeatureStatus const /* Unused for now (we don't create/delete area features)*/,
    TGenerateIDFn const & /*Unused for the same reason*/)
{
  unique_ptr<FeatureType> feature;
  auto bestScore = 0.6;  // initial score is used as a threshold.
  auto geometry = xml.GetGeometry();

  if (geometry.empty())
    MYTHROW(MigrationError, ("Feature has invalid geometry", xml));

  // This can be any point on a feature.
  auto const someFeaturePoint = geometry[0];

  sort(begin(geometry), end(geometry));  // Sort to use in set_intersection.
  auto count = 0;
  LOG(LDEBUG, ("SomePoint", someFeaturePoint));
  forEach(
      [&feature, &xml, &geometry, &count, &bestScore](FeatureType const & ft)
      {
        if (ft.GetFeatureType() != feature::GEOM_AREA)
          return;
        ++count;
        auto ftGeometry = ft.GetTriangesAsPoints(FeatureType::BEST_GEOMETRY);
        sort(begin(ftGeometry), end(ftGeometry));

        // The default comparison operator used in sort above (cmp1) and one that is
        // used in set_itersection (cmp2) are compatible in that sence that
        // cmp2(a, b) :- cmp1(a, b) and
        // cmp1(a, b) :- cmp2(a, b) || a almost equal b.
        // You can think of cmp2 as !(a >= b).
        // But cmp2 is not transitive:
        // i.e. !cmp(a, b) && !cmp(b, c) does NOT implies !cmp(a, c),
        // |a, b| < eps, |b, c| < eps.
        // This could lead to unexpected results in set_itersection (with greedy implementation),
        // but we assume such situation is very unlikely.
        auto const matched = set_intersection(begin(geometry), end(geometry),
                                              begin(ftGeometry), end(ftGeometry),
                                              CounterIterator(),
                                              [](m2::PointD const & p1, m2::PointD const & p2)
                                              {
                                                // TODO(mgsergio): Use 1e-7 everyware instead of
                                                // MercatotBounds::GetCellID2PointAbsEpsilon
                                                return p1 < p2 && !p1.EqualDxDy(p2, 1e-7);
                                              }).GetCount();

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

FeatureID MigrateFeatureIndex(osm::Editor::ForEachFeaturesNearByFn & forEach,
                              XMLFeature const & xml,
                              osm::Editor::FeatureStatus const featureStatus,
                              TGenerateIDFn const & generateID)
{
  switch (xml.GetType())
  {
  case XMLFeature::Type::Node:
    return MigrateNodeFeatureIndex(forEach, xml, featureStatus, generateID);
  case XMLFeature::Type::Way:
    return MigrateWayFeatureIndex(forEach, xml, featureStatus, generateID);
  }
}
}  // namespace editor
