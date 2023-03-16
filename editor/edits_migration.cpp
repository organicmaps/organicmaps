#include "editor/edits_migration.hpp"

#include "editor/feature_matcher.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_source.hpp"

#include "geometry/intersection_score.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <optional>

namespace editor
{
FeatureID MigrateNodeFeatureIndex(osm::Editor::ForEachFeaturesNearByFn & forEach,
                                  XMLFeature const & xml,
                                  FeatureStatus const featureStatus,
                                  GenerateIDFn const & generateID)
{
  if (featureStatus == FeatureStatus::Created)
    return generateID();

  FeatureID fid;
  auto count = 0;
  forEach(
      [&fid, &count](FeatureType const & ft) {
        if (ft.GetGeomType() != feature::GeomType::Point)
          return;
        // TODO(mgsergio): Check that ft and xml correspond to the same feature.
        fid = ft.GetID();
        ++count;
      },
      mercator::FromLatLon(xml.GetCenter()));

  if (count == 0)
    MYTHROW(MigrationError, ("No pointed features returned."));

  if (count > 1)
  {
    LOG(LWARNING,
        (count, "features returned for point", mercator::FromLatLon(xml.GetCenter())));
  }

  return fid;
}

FeatureID MigrateWayOrRelatonFeatureIndex(
    osm::Editor::ForEachFeaturesNearByFn & forEach, XMLFeature const & xml,
    FeatureStatus const /* Unused for now (we don't create/delete area features)*/,
    GenerateIDFn const & /*Unused for the same reason*/)
{
  std::optional<FeatureID> fid;
  auto bestScore = 0.6;  // initial score is used as a threshold.
  auto geometry = xml.GetGeometry();
  auto count = 0;

  if (geometry.empty())
    MYTHROW(MigrationError, ("Feature has invalid geometry", xml));

  // This can be any point on a feature.
  auto const someFeaturePoint = geometry[0];

  forEach(
      [&fid, &geometry, &count, &bestScore](FeatureType & ft) {
        if (ft.GetGeomType() != feature::GeomType::Area)
          return;
        ++count;
        auto ftGeometry = ft.GetTrianglesAsPoints(FeatureType::BEST_GEOMETRY);

        double score = 0.0;
        try
        {
          score = matcher::ScoreTriangulatedGeometries(geometry, ftGeometry);
        }
        catch (geometry::NotAPolygonException & ex)
        {
          LOG(LWARNING, (ex.Msg()));
          // Support migration for old application versions.
          // TODO(a): To remove it when version 8.0.x will no longer be supported.
          base::SortUnique(geometry);
          base::SortUnique(ftGeometry);
          score = matcher::ScoreTriangulatedGeometriesByPoints(geometry, ftGeometry);
        }

        if (score > bestScore)
        {
          bestScore = score;
          fid = ft.GetID();
        }
      },
      someFeaturePoint);

  if (count == 0)
    MYTHROW(MigrationError, ("No ways returned for point", someFeaturePoint));

  if (!fid)
  {
    MYTHROW(MigrationError,
            ("None of returned ways suffice. Possibly, the feature has been deleted."));
  }
  return *fid;
}

FeatureID MigrateFeatureIndex(osm::Editor::ForEachFeaturesNearByFn & forEach,
                              XMLFeature const & xml,
                              FeatureStatus const featureStatus,
                              GenerateIDFn const & generateID)
{
  switch (xml.GetType())
  {
  case XMLFeature::Type::Unknown:
    MYTHROW(MigrationError, ("Migration for XMLFeature::Type::Unknown is not possible"));
  case XMLFeature::Type::Node:
    return MigrateNodeFeatureIndex(forEach, xml, featureStatus, generateID);
  case XMLFeature::Type::Way:
  case XMLFeature::Type::Relation:
    return MigrateWayOrRelatonFeatureIndex(forEach, xml, featureStatus, generateID);
  }
  UNREACHABLE();
}
}  // namespace editor
