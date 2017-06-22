#pragma once

#include "partners_api/taxi_base.hpp"

#include <string>

namespace ms
{
class LatLon;
}

namespace taxi
{
namespace yandex
{
class RawApi
{
public:
  static bool GetTaxiInfo(ms::LatLon const & from, ms::LatLon const & to, std::string & result);
};

class Api : public ApiBase
{
public:
  /// Requests list of available products from Yandex. Returns request identifier immediately.
  void GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                            ProductsCallback const & successFn,
                            ErrorProviderCallback const & errorFn) override;

  /// Returns link which allows you to launch the Yandex app.
  RideRequestLinks GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                       ms::LatLon const & to) const override;
};

void MakeFromJson(std::string const & src, std::vector<taxi::Product> & products);
void SetYandexUrlForTesting(std::string const & url);
}  // namespace yandex
}  // namespace taxi
