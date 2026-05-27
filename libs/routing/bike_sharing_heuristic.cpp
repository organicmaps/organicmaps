#include "routing/bike_sharing_heuristic.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include <algorithm>

namespace routing
{
namespace
{
std::string GetProvider(FeatureType & ft)
{
  auto const network = ft.GetMetadata(feature::Metadata::FMD_NETWORK);
  if (!network.empty())
    return std::string(network);

  auto const op = ft.GetMetadata(feature::Metadata::FMD_OPERATOR);
  if (!op.empty())
    return std::string(op);

  return {};
}
}  // namespace

std::vector<BicycleRentalStation> GetNearestStations(std::vector<BicycleRentalStation> stations,
                                                     m2::PointD const & point)
{
  std::sort(stations.begin(), stations.end(),
            [&point](BicycleRentalStation const & lhs, BicycleRentalStation const & rhs)
  { return mercator::DistanceOnEarth(point, lhs.m_point) < mercator::DistanceOnEarth(point, rhs.m_point); });

  if (stations.size() > kPublicBicycleMaxStationsPerSide)
    stations.resize(kPublicBicycleMaxStationsPerSide);

  return stations;
}

bool AreBicycleRentalStationsCompatible(BicycleRentalStation const & startStation,
                                        BicycleRentalStation const & finishStation)
{
  return startStation.m_provider.empty() || finishStation.m_provider.empty() ||
         startStation.m_provider == finishStation.m_provider;
}

std::vector<BicycleRentalStation> FindBicycleRentals(MwmDataSource & dataSource, m2::PointD const & center,
                                                     double radiusMeters)
{
  std::vector<BicycleRentalStation> result;
  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(center, radiusMeters);

  auto const targetType = classif().GetTypeByPath({"amenity", "bicycle_rental"});

  dataSource.ForEachStreet([&result, targetType, &center, radiusMeters](FeatureType & ft)
  {
    bool found = false;
    ft.ForEachType([&found, targetType](uint32_t t)
    {
      if (t == targetType)
        found = true;
    });

    if (found)
    {
      m2::PointD const pt = ft.GetCenter();
      if (mercator::DistanceOnEarth(center, pt) <= radiusMeters)
        result.push_back({pt, GetProvider(ft)});
    }
  }, rect);

  return result;
}
}  // namespace routing
