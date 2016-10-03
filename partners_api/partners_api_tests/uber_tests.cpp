#include "testing/testing.hpp"

#include "geometry/latlon.hpp"

#include "partners_api/uber_api.hpp"

#include "base/logging.hpp"

UNIT_TEST(Uber_SmokeTest)
{
  ms::LatLon from(59.856464, 30.371867);
  ms::LatLon to(59.856000, 30.371000);

  {
    uber::Api uberApi;
    auto reqId = 0;
    reqId = uberApi.GetAvailableProducts(from, to, [&reqId](vector<uber::Product> const & products, size_t const requestId)
    {
      TEST(!products.empty(), ());
      TEST_EQUAL(requestId, reqId, ());

      for(auto const & product : products)
      {
        LOG(LINFO, (product.m_productId, product.m_name, product.m_time, product.m_price));
        TEST(!product.m_productId.empty() && !product.m_name.empty() && !product.m_time.empty() && !product.m_price.empty(),());
      }
    });
  }
}
