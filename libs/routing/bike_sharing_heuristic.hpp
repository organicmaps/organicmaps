#pragma once

#include "geometry/point2d.hpp"
#include "routing/data_source.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace routing
{
inline constexpr double kPublicBicycleMaxWalkRadiusM = 1500.0;
inline constexpr size_t kPublicBicycleMaxStationsPerSide = 3;

struct BicycleRentalStation
{
  m2::PointD m_point;
  std::string m_provider;
};

bool AreBicycleRentalStationsCompatible(BicycleRentalStation const & startStation,
                                        BicycleRentalStation const & finishStation);

std::vector<BicycleRentalStation> GetNearestStations(std::vector<BicycleRentalStation> stations,
                                                     m2::PointD const & point);

std::vector<BicycleRentalStation> FindBicycleRentals(MwmDataSource & dataSource, m2::PointD const & center,
                                                     double radiusMeters);
}  // namespace routing
