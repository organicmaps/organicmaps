#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/async_gui_thread.hpp"

#include "partners_api/cian_api.hpp"

#include "3party/jansson/myjansson.hpp"

using namespace partners_api;

namespace
{
UNIT_TEST(Cian_GetRentNearbyRaw)
{
  auto const result = cian::RawApi::GetRentNearby({37.402891, 55.656318, 37.840971, 55.859980});

  TEST(!result.m_data.empty(), ());

  my::Json root(result.m_data.c_str());
  TEST(json_is_object(root.get()), ());
}

UNIT_CLASS_TEST(AsyncGuiThread, Cian_GetRentNearby)
{
  ms::LatLon latlon(55.807385, 37.505554);
  uint64_t reqId = 0;

  {
    cian::Api api("http://localhost:34568/partners");
    std::vector<cian::RentPlace> result;

    reqId = api.GetRentNearby(
        latlon,
        [&reqId, &result](std::vector<cian::RentPlace> const & places, uint64_t const requestId) {
          TEST_EQUAL(reqId, requestId, ());
          result = places;
          testing::Notify();
        },
        [&reqId](int httpCode, uint64_t const requestId) {
          TEST_EQUAL(reqId, requestId, ());
          TEST(false, (httpCode));
        });

    testing::Wait();

    TEST(!result.empty(), ());
    for (auto const & r : result)
    {
      for (auto const & o : r.m_offers)
      {
        TEST(o.IsValid(), ());
      }
    }
  }
  {
    cian::Api api("incorrect url");
    std::vector<cian::RentPlace> result;
    int httpCode = -1;

    reqId = api.GetRentNearby(
        latlon,
        [&reqId, &result](std::vector<cian::RentPlace> const & places, uint64_t const requestId) {
          TEST_EQUAL(reqId, requestId, ());
          TEST(false, (requestId));
        },
        [&reqId, &httpCode](int code, uint64_t const requestId) {
          TEST_EQUAL(reqId, requestId, ());
          httpCode = code;
          testing::Notify();
        });

    testing::Wait();

    TEST_NOT_EQUAL(httpCode, -1, ());
  }
}
}  // namespace
