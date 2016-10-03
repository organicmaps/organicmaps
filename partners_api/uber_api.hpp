#pragma once

#include "base/thread.hpp"

#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace ms
{
  class LatLon;
}  // namespace ms

namespace downloader
{
  class HttpRequest;
}  // namespace downloader

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
  string m_price;
};
/// @products - vector of available products for requested route.
/// @requestId - identificator which was provided to GetAvailableProducts to identify request.
using ProductsCallback = function<void(vector<Product> const & products, size_t const requestId)>;

class Api
{
public:
  /// Requests list of available products from Uber. Returns request identificator immediately.
  size_t GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                             ProductsCallback const & fn);

  /// Returns link which allows you to launch the Uber app.
  static string GetRideRequestLink(string const & m_productId, ms::LatLon const & from, ms::LatLon const & to);

private:
  using threadDeleter = function<void(threads::SimpleThread *)>;
  unique_ptr<threads::SimpleThread, threadDeleter> m_thread;

  mutex m_mutex;
};
}  // namespace uber



