#pragma once

#include "partners_api/taxi_base.hpp"

#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace ms
{
class LatLon;
}

namespace downloader
{
class HttpRequest;
}

namespace taxi
{
namespace uber
{
extern string const kEstimatesUrl;
extern string const kProductsUrl;
/// Uber api wrapper based on synchronous http requests.
class RawApi
{
public:
  /// Returns true when http request was executed successfully and response copied into @result,
  /// otherwise returns false. The response contains the display name and other details about each
  /// product, and lists the products in the proper display order. This endpoint does not reflect
  /// real-time availability of the products.
  static bool GetProducts(ms::LatLon const & pos, string & result,
                          std::string const & url = kProductsUrl);
  /// Returns true when http request was executed successfully and response copied into @result,
  /// otherwise returns false. The response contains ETAs for all products currently available
  /// at a given location, with the ETA for each product expressed as integers in seconds. If a
  /// product returned from GetProducts is not returned from this endpoint for a given
  /// latitude/longitude pair then there are currently none of that product available to request.
  /// Call this endpoint every minute to provide the most accurate, up-to-date ETAs.
  static bool GetEstimatedTime(ms::LatLon const & pos, string & result,
                               std::string const & url = kEstimatesUrl);
  /// Returns true when http request was executed successfully and response copied into @result,
  /// otherwise returns false. The response contains an estimated price range for each product
  /// offered at a given location. The price estimate is provided as a formatted string with the
  /// full price range and the localized currency symbol.
  static bool GetEstimatedPrice(ms::LatLon const & from, ms::LatLon const & to, string & result,
                                std::string const & url = kEstimatesUrl);
};

/// Class which used for making products from http requests results.
class ProductMaker
{
public:
  void Reset(uint64_t const requestId);
  void SetTimes(uint64_t const requestId, string const & times);
  void SetPrices(uint64_t const requestId, string const & prices);
  void SetError(uint64_t const requestId, taxi::ErrorCode code);
  void MakeProducts(uint64_t const requestId, ProductsCallback const & successFn,
                    ErrorProviderCallback const & errorFn);

private:
  uint64_t m_requestId = 0;
  unique_ptr<string> m_times;
  unique_ptr<string> m_prices;
  unique_ptr<taxi::ErrorCode> m_error;
  mutex m_mutex;
};

class Api : public ApiBase
{
public:
  explicit Api(std::string const & baseUrl = kEstimatesUrl) : ApiBase(baseUrl) {}
  // ApiBase overrides:
  /// Requests list of available products from Uber.
  void GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                            ProductsCallback const & successFn,
                            ErrorProviderCallback const & errorFn) override;

  /// Returns link which allows you to launch the Uber app.
  RideRequestLinks GetRideRequestLinks(string const & productId, ms::LatLon const & from,
                                       ms::LatLon const & to) const override;

private:
  shared_ptr<ProductMaker> m_maker = make_shared<ProductMaker>();
  uint64_t m_requestId = 0;
};

void SetUberUrlForTesting(string const & url);
}  // namespace uber
}  // namespace taxi
