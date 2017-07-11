#include "testing/testing.hpp"

#include "partners_api/cian_api.hpp"

#include "3party/jansson/myjansson.hpp"

namespace
{
UNIT_TEST(Cian_GetRentNearbyRaw)
{
  string result;
  cian::RawApi::GetRentNearby({37.402891, 55.656318, 37.840971, 55.859980}, result);

  TEST(!result.empty(), ());

  my::Json root(result.c_str());
  TEST(json_is_object(root.get()), ());
}

UNIT_TEST(Cian_GetRentNearby)
{
  cian::Api api("http://localhost:34568/partners");
  m2::RectD rect(37.501365, 55.805666, 37.509873, 55.809183);

  std::vector<cian::RentPlace> result;
  uint64_t reqId = 0;
  reqId = api.GetRentNearby(rect, [&reqId, &result](std::vector<cian::RentPlace> const & places,
                                                    uint64_t const requestId) {
    TEST_EQUAL(reqId, requestId, ());
    result = places;
    testing::StopEventLoop();
  });

  testing::RunEventLoop();

  TEST(!result.empty(), ());
}
}  // namespace
