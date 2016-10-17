#pragma once

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

namespace uber
{
// Uber api wrapper based on synchronous http requests.
class RawApi
{
public:
  /// Returns information about the Uber products offered at a given location.
  /// The response includes the display name and other details about each product, and lists the
  /// products in the proper display order. This endpoint does not reflect real-time availability
  /// of the products.
  static string GetProducts(ms::LatLon const & pos);
  /// Returns ETAs for all products currently available at a given location, with the ETA for each
  /// product expressed as integers in seconds. If a product returned from GetProducts is not
  /// returned from this endpoint for a given latitude/longitude pair then there are currently none
  /// of that product available to request. Call this endpoint every minute to provide the most
  /// accurate, up-to-date ETAs.
  static string GetEstimatedTime(ms::LatLon const & pos);
  /// Returns an estimated price range for each product offered at a given location. The price
  /// estimate is provided as a formatted string with the full price range and the localized
  /// currency symbol.
  static string GetEstimatedPrice(ms::LatLon const & from, ms::LatLon const & to);
};

struct Product
{
  string m_productId;
  string m_name;
  string m_time;
  string m_price;     // for some currencies this field contains symbol of currency but not always
  string m_currency;  // currency can be empty, for ex. when m_price equal to Metered
};
/// @products - vector of available products for requested route.
/// @requestId - identificator which was provided to GetAvailableProducts to identify request.
using ProductsCallback = function<void(vector<Product> const & products, uint64_t const requestId)>;

/// Class which used for making products from http requests results.
class ProductMaker
{
public:
  void Reset(uint64_t const requestId);
  void SetTimes(uint64_t const requestId, string const & times);
  void SetPrices(uint64_t const requestId, string const & prices);
  void MakeProducts(uint64_t const requestId, ProductsCallback const & fn);

private:
  uint64_t m_requestId = 0;
  unique_ptr<string> m_times;
  unique_ptr<string> m_prices;
  mutex m_mutex;
};

struct RideRequestLinks
{
  string m_deepLink;
  string m_universalLink;
};

class Api
{
public:
  /// Requests list of available products from Uber. Returns request identificator immediately.
  uint64_t GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                ProductsCallback const & fn);

  /// Returns link which allows you to launch the Uber app.
  static RideRequestLinks GetRideRequestLinks(string const & productId, ms::LatLon const & from,
                                              ms::LatLon const & to);

private:
  shared_ptr<ProductMaker> m_maker = make_shared<ProductMaker>();
  uint64_t m_requestId = 0;
};
}  // namespace uber
