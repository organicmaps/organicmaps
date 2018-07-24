#include "routing/geometry.hpp"

#include "routing/routing_exceptions.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/altitude_loader.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

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
                     shared_ptr<VehicleModelInterface> vehicleModel, bool loadAltitudes);

  // GeometryLoader overrides:
  void Load(uint32_t featureId, RoadGeometry & road) override;

private:
  shared_ptr<VehicleModelInterface> m_vehicleModel;
  FeaturesLoaderGuard m_guard;
  string const m_country;
  feature::AltitudeLoader m_altitudeLoader;
  bool const m_loadAltitudes;
};

GeometryLoaderImpl::GeometryLoaderImpl(DataSource const & dataSource,
                                       MwmSet::MwmHandle const & handle,
                                       shared_ptr<VehicleModelInterface> vehicleModel,
                                       bool loadAltitudes)
  : m_vehicleModel(move(vehicleModel))
  , m_guard(dataSource, handle.GetId())
  , m_country(handle.GetInfo()->GetCountryName())
  , m_altitudeLoader(dataSource, handle.GetId())
  , m_loadAltitudes(loadAltitudes)
{
  CHECK(handle.IsAlive(), ());
  CHECK(m_vehicleModel, ());
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

  road.Load(*m_vehicleModel, feature, altitudes);
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
  shared_ptr<VehicleModelInterface> m_vehicleModel;
};

FileGeometryLoader::FileGeometryLoader(string const & fileName,
                                       shared_ptr<VehicleModelInterface> vehicleModel)
  : m_featuresVector(FilesContainerR(make_unique<FileReader>(fileName)))
  , m_vehicleModel(vehicleModel)
{
  CHECK(m_vehicleModel, ());
}

void FileGeometryLoader::Load(uint32_t featureId, RoadGeometry & road)
{
  FeatureType feature;
  m_featuresVector.GetVector().GetByIndex(featureId, feature);
  feature.ParseGeometry(FeatureType::BEST_GEOMETRY);
  road.Load(*m_vehicleModel, feature, nullptr /* altitudes */);
}
}  // namespace

namespace routing
{
// RoadGeometry ------------------------------------------------------------------------------------
RoadGeometry::RoadGeometry(bool oneWay, double weightSpeedKMpH, double etaSpeedKMpH,
                           Points const & points)
  : m_speed{weightSpeedKMpH, etaSpeedKMpH}, m_isOneWay{oneWay}, m_valid{true}
{
  ASSERT_GREATER(weightSpeedKMpH, 0.0, ());
  ASSERT_GREATER(etaSpeedKMpH, 0.0, ());

  m_junctions.reserve(points.size());
  for (auto const & point : points)
    m_junctions.emplace_back(point, feature::kDefaultAltitudeMeters);
}

void RoadGeometry::Load(VehicleModelInterface const & vehicleModel, FeatureType & feature,
                        feature::TAltitudes const * altitudes)
{
  CHECK(altitudes == nullptr || altitudes->size() == feature.GetPointsCount(), ());

  m_valid = vehicleModel.IsRoad(feature);
  m_isOneWay = vehicleModel.IsOneWay(feature);
  m_speed = vehicleModel.GetSpeed(feature);
  m_isPassThroughAllowed = vehicleModel.IsPassThroughAllowed(feature);

  m_junctions.clear();
  m_junctions.reserve(feature.GetPointsCount());
  for (size_t i = 0; i < feature.GetPointsCount(); ++i)
  {
    m_junctions.emplace_back(feature.GetPoint(i),
                             altitudes ? (*altitudes)[i] : feature::kDefaultAltitudeMeters);
  }

  if (m_valid && m_speed.m_weight <= 0.0)
  {
    auto const & id = feature.GetID();
    CHECK(!m_junctions.empty(), ("mwm:", id.GetMwmName(), ", featureId:", id.m_index));
    auto const begin = MercatorBounds::ToLatLon(m_junctions.front().GetPoint());
    auto const end = MercatorBounds::ToLatLon(m_junctions.back().GetPoint());
    LOG(LERROR, ("Invalid speed", m_speed.m_weight, "mwm:", id.GetMwmName(),
                 ", featureId:", id.m_index, ", begin:", begin, "end:", end));
    m_valid = false;
  }
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
                                                  bool loadAltitudes)
{
  CHECK(handle.IsAlive(), ());
  return make_unique<GeometryLoaderImpl>(dataSource, handle, vehicleModel, loadAltitudes);
}

// static
unique_ptr<GeometryLoader> GeometryLoader::CreateFromFile(string const & fileName,
                                                          shared_ptr<VehicleModelInterface> vehicleModel)
{
  return make_unique<FileGeometryLoader>(fileName, vehicleModel);
}
}  // namespace routing
