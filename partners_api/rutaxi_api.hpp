#pragma once

#include "partners_api/taxi_base.hpp"
#include "partners_api/taxi_delegate.hpp"

#include "base/visitor.hpp"

#include <string>
#include <unordered_map>

namespace ms
{
class LatLon;
}

namespace taxi
{
namespace rutaxi
{
struct City
{
  std::string m_id;
  std::string m_currency;
};

using CityMapping = std::unordered_map<std::string, City>;

struct Object
{
  std::string m_id;
  std::string m_house;
  std::string m_title;
};

struct OsmToId
{
  std::string m_osmName;
  std::string m_id;
};

extern std::string const kTaxiInfoUrl;
/// RuTaxi api wrapper is based on synchronous http requests.
class RawApi
{
public:
  static bool GetNearObject(ms::LatLon const & pos, std::string const & city, std::string & result,
                            std::string const & baseUrl = kTaxiInfoUrl);

  static bool GetCost(Object const & from, Object const & to, std::string const & city,
                      std::string & result, std::string const & baseUrl = kTaxiInfoUrl);
};

/// Class which is used for making products from http requests results.
class Api : public ApiBase
{
public:
  explicit Api(std::string const & baseUrl = kTaxiInfoUrl);

  void SetDelegate(Delegate * delegate);
  // ApiBase overrides:
  /// Requests list of available products from RuTaxi.
  void GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                            ProductsCallback const & successFn,
                            ErrorProviderCallback const & errorFn) override;

  /// Returns link which allows you to launch the RuTaxi app.
  RideRequestLinks GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                       ms::LatLon const & to) const override;

private:
  // Non-owning delegate pointer
  Delegate * m_delegate = nullptr;
  CityMapping m_cityMapping;
};

void MakeNearObject(std::string const & src, Object & dst);
void MakeProducts(std::string const & src, Object const & from, Object const & to,
                  City const & city, std::vector<taxi::Product> & products);
CityMapping LoadCityMapping();
}  // namespace rutaxi
}  // namespace taxi
