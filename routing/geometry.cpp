#include "geometry.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

namespace
{
using namespace routing;

double constexpr kMPH2MPS = 1000.0 / (60 * 60);

inline double TimeBetweenSec(m2::PointD const & from, m2::PointD const & to, double speedMPS)
{
  ASSERT_GREATER(speedMPS, 0.0, ());

  double const distanceM = MercatorBounds::DistanceOnEarth(from, to);
  return distanceM / speedMPS;
}

class GeometryImpl final : public Geometry
{
public:
  GeometryImpl(Index const & index, MwmSet::MwmId const & mwmId,
               shared_ptr<IVehicleModel> vehicleModel);

  // Geometry overrides:
  bool IsRoad(uint32_t featureId) const override;
  bool IsOneWay(uint32_t featureId) const override;
  m2::PointD const & GetPoint(FSegId fseg) const override;
  double CalcEdgesWeight(uint32_t featureId, uint32_t pointStart,
                         uint32_t pointFinish) const override;
  double CalcHeuristic(FSegId from, FSegId to) const override;

private:
  FeatureType const & GetFeature(uint32_t featureId) const;
  FeatureType const & LoadFeature(uint32_t featureId) const;

  Index const & m_index;
  MwmSet::MwmId const m_mwmId;
  shared_ptr<IVehicleModel> m_vehicleModel;
  double const m_maxSpeedMPS;
  mutable map<uint32_t, FeatureType> m_features;
};

GeometryImpl::GeometryImpl(Index const & index, MwmSet::MwmId const & mwmId,
                           shared_ptr<IVehicleModel> vehicleModel)
  : m_index(index)
  , m_mwmId(mwmId)
  , m_vehicleModel(vehicleModel)
  , m_maxSpeedMPS(vehicleModel->GetMaxSpeed() * kMPH2MPS)
{
}

bool GeometryImpl::IsRoad(uint32_t featureId) const
{
  return m_vehicleModel->IsRoad(GetFeature(featureId));
}

bool GeometryImpl::IsOneWay(uint32_t featureId) const
{
  return m_vehicleModel->IsOneWay(GetFeature(featureId));
}

m2::PointD const & GeometryImpl::GetPoint(FSegId fseg) const
{
  return GetFeature(fseg.GetFeatureId()).GetPoint(fseg.GetSegId());
}

double GeometryImpl::CalcEdgesWeight(uint32_t featureId, uint32_t pointFrom, uint32_t pointTo) const
{
  uint32_t const start = min(pointFrom, pointTo);
  uint32_t const finish = max(pointFrom, pointTo);
  FeatureType const & feature = GetFeature(featureId);

  ASSERT_LESS(finish, feature.GetPointsCount(), ());

  double result = 0.0;
  double const speedMPS = m_vehicleModel->GetSpeed(feature) * kMPH2MPS;
  for (uint32_t i = start; i < finish; ++i)
  {
    result += TimeBetweenSec(feature.GetPoint(i), feature.GetPoint(i + 1), speedMPS);
  }

  return result;
}

double GeometryImpl::CalcHeuristic(FSegId from, FSegId to) const
{
  return TimeBetweenSec(GetPoint(from), GetPoint(to), m_maxSpeedMPS);
}

FeatureType const & GeometryImpl::GetFeature(uint32_t featureId) const
{
  auto it = m_features.find(featureId);
  if (it != m_features.end())
    return it->second;

  return LoadFeature(featureId);
}

FeatureType const & GeometryImpl::LoadFeature(uint32_t featureId) const
{
  Index::FeaturesLoaderGuard guard(m_index, m_mwmId);
  FeatureType & feature = m_features[featureId];
  bool const isFound = guard.GetFeatureByIndex(featureId, feature);
  ASSERT(isFound, ("Feature", featureId, "not found"));
  if (isFound)
    feature.ParseGeometry(FeatureType::BEST_GEOMETRY);

  return feature;
}
}  // namespace

namespace routing
{
unique_ptr<Geometry> CreateGeometry(Index const & index, MwmSet::MwmId const & mwmId,
                                    shared_ptr<IVehicleModel> vehicleModel)
{
  return make_unique<GeometryImpl>(index, mwmId, vehicleModel);
}
}  // namespace routing
