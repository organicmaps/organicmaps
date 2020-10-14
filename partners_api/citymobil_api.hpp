#pragma once

#include "partners_api/taxi_base.hpp"

#include "coding/serdes_json.hpp"

#include <string>
#include <vector>

namespace ms
{
class LatLon;
}

namespace taxi
{
namespace citymobil
{
extern std::string const kBaseUrl;
/// Citymobil api wrapper based on synchronous http requests.
class RawApi
{
public:
  using TariffGroups = std::vector<uint8_t>;

  struct Position
  {
    DECLARE_VISITOR(visitor(m_lat, "latitude"), visitor(m_lon, "longitude"));

    double m_lat = 0.0;
    double m_lon = 0.0;
  };

  class CalculatePriceBody
  {
  public:
    CalculatePriceBody(ms::LatLon const & from, ms::LatLon const & to, TariffGroups const & tariffs)
      : m_pickup{from.m_lat, from.m_lon}
      , m_dropoff{to.m_lat, to.m_lon}
      , m_tariffGroups(tariffs)
    {
    }

    DECLARE_VISITOR(visitor(m_pickup, "pickup"), visitor(m_dropoff, "dropoff"),
                    visitor(m_tariffGroups, "tariff_group"), visitor(m_waypoints, "waypoints"));

  private:
    Position m_pickup;
    Position m_dropoff;
    TariffGroups m_tariffGroups;
    // Required api parameter but it is not used in our app.
    std::vector<Position> m_waypoints;
  };

  class SupportedTariffsBody
  {
  public:
    SupportedTariffsBody(ms::LatLon const & pos) : m_pickup{pos.m_lat, pos.m_lon} {}

    DECLARE_VISITOR(visitor(m_pickup, "pickup"));

  private:
    Position m_pickup;
  };

  static bool GetSupportedTariffs(SupportedTariffsBody const & body, std::string & result,
                                  std::string const & url = kBaseUrl);
  static bool CalculatePrice(CalculatePriceBody const & body, std::string & result,
                             std::string const & url = kBaseUrl);
};

/// Class which used for making products from http requests results.
class Api : public ApiBase
{
public:
  explicit Api(std::string const & baseUrl = kBaseUrl) : ApiBase(baseUrl) {}
  // ApiBase overrides:
  /// Requests list of available products from Citymobil.
  void GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                            ProductsCallback const & successFn,
                            ErrorProviderCallback const & errorFn) override;

  /// Returns link which allows you to launch the Citymobil app.
  RideRequestLinks GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                       ms::LatLon const & to) const override;
};

RawApi::TariffGroups MakeTariffGroupsFromJson(std::string const & src);
std::vector<taxi::Product> MakeProductsFromJson(std::string const & src);
}  // namespace citymobil
}  // namespace taxi
