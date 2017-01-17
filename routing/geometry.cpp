#include "routing/geometry.hpp"

#include "routing/routing_exceptions.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include "std/string.hpp"

using namespace routing;

namespace
{
class GeometryLoaderImpl final : public GeometryLoader
{
public:
  GeometryLoaderImpl(Index const & index, MwmSet::MwmId const & mwmId, string const & country,
                     shared_ptr<IVehicleModel> vehicleModel);

  // GeometryLoader overrides:
  virtual void Load(uint32_t featureId, RoadGeometry & road) const override;

private:
  shared_ptr<IVehicleModel> m_vehicleModel;
  Index::FeaturesLoaderGuard m_guard;
  string const m_country;
};

GeometryLoaderImpl::GeometryLoaderImpl(Index const & index, MwmSet::MwmId const & mwmId,
                                       string const & country,
                                       shared_ptr<IVehicleModel> vehicleModel)
  : m_vehicleModel(vehicleModel), m_guard(index, mwmId), m_country(country)
{
  ASSERT(m_vehicleModel, ());
}

void GeometryLoaderImpl::Load(uint32_t featureId, RoadGeometry & road) const
{
  FeatureType feature;
  bool const isFound = m_guard.GetFeatureByIndex(featureId, feature);
  if (!isFound)
    MYTHROW(RoutingException, ("Feature", featureId, "not found in ", m_country));

  feature.ParseGeometry(FeatureType::BEST_GEOMETRY);
  road.Load(*m_vehicleModel, feature);
}
}  // namespace

namespace routing
{
// RoadGeometry ------------------------------------------------------------------------------------
RoadGeometry::RoadGeometry(bool oneWay, double speed, Points const & points)
  : m_points(points), m_speed(speed), m_isOneWay(oneWay), m_valid(true)
{
  ASSERT_GREATER(speed, 0.0, ());
}

void RoadGeometry::Load(IVehicleModel const & vehicleModel, FeatureType const & feature)
{
  m_valid = vehicleModel.IsRoad(feature);
  m_isOneWay = vehicleModel.IsOneWay(feature);
  m_speed = vehicleModel.GetSpeed(feature);

  m_points.clear();
  m_points.reserve(feature.GetPointsCount());
  for (size_t i = 0; i < feature.GetPointsCount(); ++i)
    m_points.emplace_back(feature.GetPoint(i));

  if (m_valid && m_speed <= 0.0)
  {
    auto const & id = feature.GetID();
    CHECK(!m_points.empty(), ("mwm:", id.GetMwmName(), ", featureId:", id.m_index));
    auto const begin = MercatorBounds::ToLatLon(m_points.front());
    auto const end = MercatorBounds::ToLatLon(m_points.back());
    LOG(LERROR, ("Invalid speed", m_speed, "mwm:", id.GetMwmName(), ", featureId:", id.m_index,
                 ", begin:", begin, "end:", end));
    m_valid = false;
  }
}

// Geometry ----------------------------------------------------------------------------------------
Geometry::Geometry(unique_ptr<GeometryLoader> loader) : m_loader(move(loader))
{
  ASSERT(m_loader, ());
}

RoadGeometry const & Geometry::GetRoad(uint32_t featureId)
{
  auto const & it = m_roads.find(featureId);
  if (it != m_roads.cend())
    return it->second;

  RoadGeometry & road = m_roads[featureId];
  m_loader->Load(featureId, road);
  return road;
}

// static
unique_ptr<GeometryLoader> GeometryLoader::Create(Index const & index, MwmSet::MwmId const & mwmId,
                                                  shared_ptr<IVehicleModel> vehicleModel)
{
  CHECK(mwmId.IsAlive(), ());
  return make_unique<GeometryLoaderImpl>(index, mwmId, mwmId.GetInfo()->GetCountryName(),
                                         vehicleModel);
}
}  // namespace routing
