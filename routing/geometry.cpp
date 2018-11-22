#include "routing/geometry.hpp"

#include "routing/city_roads.hpp"
#include "routing/maxspeeds.hpp"
#include "routing/routing_exceptions.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/data_source.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <string>

using namespace routing;
using namespace std;

namespace
{
// @TODO(bykoianko) Consider setting cache size based on available memory.
// Maximum road geometry cache size in items.
size_t constexpr kRoadsCacheSize = 5000;

// GeometryLoaderImpl ------------------------------------------------------------------------------
class GeometryLoaderImpl final : public GeometryLoader
{
public:
  GeometryLoaderImpl(DataSource const & dataSource, MwmSet::MwmHandle const & handle,
                     shared_ptr<VehicleModelInterface> vehicleModel,
                     AttrLoader attrLoader, bool loadAltitudes);

  // GeometryLoader overrides:
  void Load(uint32_t featureId, RoadGeometry & road) override;

private:
  shared_ptr<VehicleModelInterface> m_vehicleModel;
  AttrLoader m_attrLoader;
  FeaturesLoaderGuard m_guard;
  string const m_country;
  feature::AltitudeLoader m_altitudeLoader;
  bool const m_loadAltitudes;
};

GeometryLoaderImpl::GeometryLoaderImpl(DataSource const & dataSource,
                                       MwmSet::MwmHandle const & handle,
                                       shared_ptr<VehicleModelInterface> vehicleModel,
                                       AttrLoader attrLoader, bool loadAltitudes)
  : m_vehicleModel(move(vehicleModel))
  , m_attrLoader(move(attrLoader))
  , m_guard(dataSource, handle.GetId())
  , m_country(handle.GetInfo()->GetCountryName())
  , m_altitudeLoader(dataSource, handle.GetId())
  , m_loadAltitudes(loadAltitudes)
{
  CHECK(handle.IsAlive(), ());
  CHECK(m_vehicleModel, ());
  CHECK(m_attrLoader.m_cityRoads, ());
  CHECK(m_attrLoader.m_maxspeeds, ());
}

void GeometryLoaderImpl::Load(uint32_t featureId, RoadGeometry & road)
{
  FeatureType feature;
  bool const isFound = m_guard.GetFeatureByIndex(featureId, feature);
  if (!isFound)
    MYTHROW(RoutingException, ("Feature", featureId, "not found in ", m_country));

  feature.ParseGeometry(FeatureType::BEST_GEOMETRY);

  feature::TAltitudes const * altitudes = nullptr;
  if (m_loadAltitudes)
    altitudes = &(m_altitudeLoader.GetAltitudes(featureId, feature.GetPointsCount()));

  road.Load(*m_vehicleModel, feature, altitudes, m_attrLoader.m_cityRoads->IsCityRoad(featureId),
            m_attrLoader.m_maxspeeds->GetMaxspeed(featureId));
  m_altitudeLoader.ClearCache();
}

// FileGeometryLoader ------------------------------------------------------------------------------
class FileGeometryLoader final : public GeometryLoader
{
public:
  FileGeometryLoader(string const & fileName, shared_ptr<VehicleModelInterface> vehicleModel);

  // GeometryLoader overrides:
  void Load(uint32_t featureId, RoadGeometry & road) override;

private:
  FeaturesVectorTest m_featuresVector;
  CityRoads m_cityRoads;
  Maxspeeds m_maxspeeds;
  shared_ptr<VehicleModelInterface> m_vehicleModel;
};

FileGeometryLoader::FileGeometryLoader(string const & fileName,
                                       shared_ptr<VehicleModelInterface> vehicleModel)
  : m_featuresVector(FilesContainerR(make_unique<FileReader>(fileName)))
  , m_vehicleModel(vehicleModel)
{
  auto const cont = FilesContainerR(make_unique<FileReader>(fileName));

  try
  {
    if (cont.IsExist(CITY_ROADS_FILE_TAG))
      LoadCityRoads(fileName, cont.GetReader(CITY_ROADS_FILE_TAG), m_cityRoads);

    if (cont.IsExist(MAXSPEEDS_FILE_TAG))
      LoadMaxspeeds(cont.GetReader(MAXSPEEDS_FILE_TAG), m_maxspeeds);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("File", cont.GetFileName(), "Error while reading", CITY_ROADS_FILE_TAG, "or",
                 MAXSPEEDS_FILE_TAG, "section.", e.Msg()));
  }

  CHECK(m_vehicleModel, ());
}

void FileGeometryLoader::Load(uint32_t featureId, RoadGeometry & road)
{
  FeatureType feature;
  m_featuresVector.GetVector().GetByIndex(featureId, feature);
  feature.ParseGeometry(FeatureType::BEST_GEOMETRY);
  // Note. If FileGeometryLoader is used for generation cross mwm section for bicycle or
  // pedestrian routing |altitudes| should be used here.
  road.Load(*m_vehicleModel, feature, nullptr /* altitudes */, m_cityRoads.IsCityRoad(featureId),
            m_maxspeeds.GetMaxspeed(featureId));
}
}  // namespace

namespace routing
{
// RoadGeometry ------------------------------------------------------------------------------------
RoadGeometry::RoadGeometry(bool oneWay, double weightSpeedKMpH, double etaSpeedKMpH,
                           Points const & points)
  : m_forwardSpeed{weightSpeedKMpH, etaSpeedKMpH}
  , m_backwardSpeed(m_forwardSpeed)
  , m_isOneWay(oneWay)
  , m_valid(true)
{
  ASSERT_GREATER(weightSpeedKMpH, 0.0, ());
  ASSERT_GREATER(etaSpeedKMpH, 0.0, ());

  m_junctions.reserve(points.size());
  for (auto const & point : points)
    m_junctions.emplace_back(point, feature::kDefaultAltitudeMeters);
}

void RoadGeometry::Load(VehicleModelInterface const & vehicleModel, FeatureType & feature,
                        feature::TAltitudes const * altitudes, bool inCity, Maxspeed const & maxspeed)
{
  CHECK(altitudes == nullptr || altitudes->size() == feature.GetPointsCount(), ());

  m_valid = vehicleModel.IsRoad(feature);
  m_isOneWay = vehicleModel.IsOneWay(feature);
  m_forwardSpeed = vehicleModel.GetSpeed(feature, {true /* forward */, inCity, maxspeed});
  m_backwardSpeed = vehicleModel.GetSpeed(feature, {false /* forward */, inCity, maxspeed});
  m_isPassThroughAllowed = vehicleModel.IsPassThroughAllowed(feature);

  m_junctions.clear();
  m_junctions.reserve(feature.GetPointsCount());
  for (size_t i = 0; i < feature.GetPointsCount(); ++i)
  {
    m_junctions.emplace_back(feature.GetPoint(i),
                             altitudes ? (*altitudes)[i] : feature::kDefaultAltitudeMeters);
  }

  if (m_valid && (!m_forwardSpeed.IsValid() || !m_backwardSpeed.IsValid()))
  {
    auto const & id = feature.GetID();
    CHECK(!m_junctions.empty(), ("mwm:", id.GetMwmName(), ", featureId:", id.m_index));
    auto const begin = MercatorBounds::ToLatLon(m_junctions.front().GetPoint());
    auto const end = MercatorBounds::ToLatLon(m_junctions.back().GetPoint());
    LOG(LERROR,
        ("Invalid speed m_forwardSpeed:", m_forwardSpeed, "m_backwardSpeed:", m_backwardSpeed,
         "mwm:", id.GetMwmName(), ", featureId:", id.m_index, ", begin:", begin, "end:", end));
    m_valid = false;
  }
}

VehicleModelInterface::SpeedKMpH const & RoadGeometry::GetSpeed(bool forward) const
{
  return forward ? m_forwardSpeed : m_backwardSpeed;
}

// Geometry ----------------------------------------------------------------------------------------
Geometry::Geometry(unique_ptr<GeometryLoader> loader)
    : m_loader(move(loader))
    , m_featureIdToRoad(make_unique<FifoCache<uint32_t, RoadGeometry>>(
        kRoadsCacheSize,
        [this](uint32_t featureId, RoadGeometry & road) { m_loader->Load(featureId, road); }))
{
  CHECK(m_loader, ());
}

RoadGeometry const & Geometry::GetRoad(uint32_t featureId)
{
  ASSERT(m_featureIdToRoad, ());
  ASSERT(m_loader, ());

  return m_featureIdToRoad->GetValue(featureId);
}

// static
unique_ptr<GeometryLoader> GeometryLoader::Create(DataSource const & dataSource,
                                                  MwmSet::MwmHandle const & handle,
                                                  shared_ptr<VehicleModelInterface> vehicleModel,
                                                  AttrLoader && attrLoader,
                                                  bool loadAltitudes)
{
  CHECK(handle.IsAlive(), ());
  return make_unique<GeometryLoaderImpl>(dataSource, handle, vehicleModel, move(attrLoader),
                                         loadAltitudes);
}

// static
unique_ptr<GeometryLoader> GeometryLoader::CreateFromFile(string const & fileName,
                                                          shared_ptr<VehicleModelInterface> vehicleModel)
{
  return make_unique<FileGeometryLoader>(fileName, vehicleModel);
}
}  // namespace routing
