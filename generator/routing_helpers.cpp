#include "generator/routing_helpers.hpp"

#include "generator/feature_builder.hpp"
#include "generator/utils.hpp"

#include "routing/routing_helpers.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

namespace routing
{

template <class ToDo>
void ForEachWayFromFile(std::string const & filename, ToDo && toDo)
{
  using namespace generator;
  CHECK(ForEachOsmId2FeatureId(filename,
      [&](CompositeId const & compositeOsmId, uint32_t featureId)
      {
        auto const osmId = compositeOsmId.m_mainId;
        if (osmId.GetType() == base::GeoObjectId::Type::ObsoleteOsmWay)
          toDo(featureId, osmId);
      }), ("Can't load osm id mapping from", filename));
}

void AddFeatureId(base::GeoObjectId osmId, uint32_t featureId,
                  OsmIdToFeatureIds & osmIdToFeatureIds)
{
  osmIdToFeatureIds[osmId].push_back(featureId);
}

void ParseWaysOsmIdToFeatureIdMapping(std::string const & osmIdsToFeatureIdPath,
                                      OsmIdToFeatureIds & osmIdToFeatureIds)
{
  ForEachWayFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, base::GeoObjectId osmId)
  {
    AddFeatureId(osmId, featureId, osmIdToFeatureIds);
  });
}

void ParseWaysFeatureIdToOsmIdMapping(std::string const & osmIdsToFeatureIdPath,
                                      FeatureIdToOsmId & featureIdToOsmId)
{
  featureIdToOsmId.clear();

  ForEachWayFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, base::GeoObjectId const & osmId)
  {
    auto const emplaced = featureIdToOsmId.emplace(featureId, osmId);
    CHECK(emplaced.second, ("Feature id", featureId, "is included in two osm ids:", emplaced.first->second, osmId));
  });
}

class OsmWay2FeaturePointImpl : public OsmWay2FeaturePoint
{
  OsmIdToFeatureIds m_osm2features;
  generator::FeatureGetter m_featureGetter;

public:
  OsmWay2FeaturePointImpl(std::string const & dataFilePath, std::string const & osmIdsToFeatureIdsPath)
    : m_featureGetter(dataFilePath)
  {
    ParseWaysOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, m_osm2features);
  }

  virtual void ForEachFeature(uint64_t wayID, std::function<void (uint32_t)> const & fn) override
  {
    auto it = m_osm2features.find(base::MakeOsmWay(wayID));
    if (it != m_osm2features.end())
    {
      for (uint32_t featureID : it->second)
        fn(featureID);
    }
  }

  virtual void ForEachNodeIdx(uint64_t wayID, uint32_t candidateIdx, m2::PointU pt,
                              std::function<void(uint32_t, uint32_t)> const & fn) override
  {
    auto it = m_osm2features.find(base::MakeOsmWay(wayID));
    if (it == m_osm2features.end())
      return;

    auto const & fIDs = it->second;
    if (fIDs.size() == 1)
    {
      // Fast lane when mapped feature wasn't changed.
      fn(fIDs[0], candidateIdx);
      return;
    }

    auto const fullRect = mercator::Bounds::FullRect();

    for (uint32_t featureID : fIDs)
    {
      auto ft = m_featureGetter.GetFeatureByIndex(featureID);
      CHECK(ft, (featureID));
      CHECK(ft->GetGeomType() == feature::GeomType::Line, (featureID));

      // Converison should work with the same logic as in WayNodesMapper::EncodePoint.
      auto const mercatorPt = PointUToPointD(pt, kPointCoordBits, fullRect);
      double minSquareDist = 1.0E6;

      ft->ParseGeometry(FeatureType::BEST_GEOMETRY);

      uint32_t const count = ft->GetPointsCount();
      uint32_t resIdx = 0;
      for (uint32_t i = 0; i < count; ++i)
      {
        auto const p = ft->GetPoint(i);
        double const d = p.SquaredLength(mercatorPt);
        if (d < minSquareDist)
        {
          minSquareDist = d;
          resIdx = i;
        }
      }

      // Final check to ensure that we have got exact point with Feature coordinates encoding.
      if (PointDToPointU(ft->GetPoint(resIdx), kFeatureSorterPointCoordBits, fullRect) ==
          PointDToPointU(mercatorPt, kFeatureSorterPointCoordBits, fullRect))
      {
        fn(featureID, resIdx);
      }
    }
  }
};

std::unique_ptr<OsmWay2FeaturePoint> CreateWay2FeatureMapper(
    std::string const & dataFilePath, std::string const & osmIdsToFeatureIdsPath)
{
  return std::make_unique<OsmWay2FeaturePointImpl>(dataFilePath, osmIdsToFeatureIdsPath);
}

bool IsRoadWay(feature::FeatureBuilder const & fb)
{
  return fb.IsLine() && IsRoad(fb.GetTypes());
}
}  // namespace routing
