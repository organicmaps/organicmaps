#pragma once

#include "partners_api/taxi_base.hpp"
#include "partners_api/yandex_api.hpp"

namespace taxi::yango
{
class Api : public yandex::Api
{
public:
  explicit Api(std::string const & baseUrl = yandex::kTaxiInfoUrl) : yandex::Api(baseUrl) {}

  /// Returns link which allows you to launch the Yandex app.
  RideRequestLinks GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                       ms::LatLon const & to) const override;
};
}  // namespace taxi::yango
