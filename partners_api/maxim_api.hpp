#pragma once

#include "partners_api/taxi_base.hpp"

#include <string>

namespace ms
{
class LatLon;
}

namespace taxi
{
namespace maxim
{
extern std::string const kTaxiInfoUrl;
/// Maxim api wrapper is based on synchronous http requests.
class RawApi
{
public:
  static bool GetTaxiInfo(ms::LatLon const & from, ms::LatLon const & to, std::string & result,
                          std::string const & url = kTaxiInfoUrl);
};

/// Class which is used for making products from http requests results.
class Api : public ApiBase
{
public:
  explicit Api(std::string const & baseUrl = kTaxiInfoUrl) : ApiBase(baseUrl) {}
  // ApiBase overrides:
  /// Requests list of available products from Maxim.
  void GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                            ProductsCallback const & successFn,
                            ErrorProviderCallback const & errorFn) override;

  /// Returns link which allows you to launch the Maxim app.
  RideRequestLinks GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                       ms::LatLon const & to) const override;
};

void MakeFromJson(std::string const & src, std::vector<taxi::Product> & products);
}  // namespace maxim
}  // namespace taxi
