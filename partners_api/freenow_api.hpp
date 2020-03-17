#pragma once

#include "partners_api/taxi_base.hpp"

#include <mutex>
#include <string>

namespace ms
{
class LatLon;
}

namespace taxi
{
namespace freenow
{
extern std::string const kTaxiEndpoint;
/// "Free now" api wrapper based on synchronous http requests.
class RawApi
{
public:
  static bool GetAccessToken(std::string & result, std::string const & url = kTaxiEndpoint);
  static bool GetServiceTypes(ms::LatLon const & from, ms::LatLon const & to,
                              std::string const & token, std::string & result,
                              std::string const & url = kTaxiEndpoint);
};

class SafeToken
{
public:
  struct Token
  {
    std::string m_token;
    std::chrono::steady_clock::time_point m_expiredTime;
  };

  SafeToken() = default;
  void Set(Token const & token);
  Token Get() const;

private:
  mutable std::mutex m_mutex;
  Token m_token;
};

/// Class which used for making products from http requests results.
class Api : public ApiBase
{
public:
  explicit Api(std::string const & baseUrl = kTaxiEndpoint) : ApiBase(baseUrl) {}
  // ApiBase overrides:
  /// Requests list of available products from "Free now".
  void GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                            ProductsCallback const & successFn,
                            ErrorProviderCallback const & errorFn) override;

  /// Returns link which allows you to launch the "Free now" app.
  RideRequestLinks GetRideRequestLinks(std::string const & productId, ms::LatLon const & from,
                                       ms::LatLon const & to) const override;

private:
  SafeToken m_accessToken;
};

SafeToken::Token MakeTokenFromJson(std::string const & src);
std::vector<taxi::Product> MakeProductsFromJson(std::string const & src);
}  // namespace freenow
}  // namespace taxi
