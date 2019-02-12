#include "testing/testing.hpp"

#include "partners_api/rutaxi_api.hpp"

#include "platform/platform.hpp"

#include "geometry/latlon.hpp"

#include <string>

namespace
{
class DelegateForTesting : public taxi::Delegate
{
public:
  storage::CountriesVec GetCountryIds(m2::PointD const & point) override { return {""}; }
  std::string GetCityName(m2::PointD const & point) override { return ""; }
  storage::CountryId GetMwmId(m2::PointD const & point) override { return ""; }
};

using Runner = Platform::ThreadRunner;

auto const kNearObject = R"(
{
  "success": true,
  "time": "2018-09-07 16:20:52",
  "data": {
    "objects": [
      {
        "id": "467",
        "house": "6ст3",
        "name": "Булатниковский проезд",
        "metatype": 0
      }
    ],
    "status": 220
  }
})";

UNIT_TEST(RuTaxi_GetNearObject)
{
  ms::LatLon pos(55.592522, 37.653501);
  std::string result;
  taxi::rutaxi::RawApi::GetNearObject(pos, "moscow", result);
  TEST(!result.empty(), ());
  LOG(LINFO, (result));

  base::Json root(result.c_str());

  TEST(json_is_object(root.get()), ());

  bool success = false;
  FromJSONObject(root.get(), "success", success);

  TEST(success, ());
}

UNIT_TEST(RuTaxi_GetCost)
{
  taxi::rutaxi::Object from = {"3440", "2"};
  taxi::rutaxi::Object to = {"467", "6ст3"};
  std::string result;
  taxi::rutaxi::RawApi::GetCost(from, to, "moscow", result);
  TEST(!result.empty(), ());
  LOG(LINFO, (result));

  base::Json root(result.c_str());

  TEST(json_is_object(root.get()), ());

  bool success = false;
  FromJSONObject(root.get(), "success", success);

  TEST(success, ());
}

UNIT_TEST(RuTaxi_MakeNearObject)
{
  taxi::rutaxi::Object dst;
  taxi::rutaxi::MakeNearObject(kNearObject, dst);
  TEST_EQUAL(dst.m_id, "467", ());
  TEST_EQUAL(dst.m_house, "6ст3", ());
}

UNIT_TEST(RuTaxi_MakeProducts)
{
  auto const src = R"(
  {
    "success": true,
    "time": "2018-09-07 16:20:53",
    "data": {
      "cost": "1005",
      "status": 220
    }
  })";

  taxi::rutaxi::Object from;
  taxi::rutaxi::MakeNearObject(kNearObject, from);
  taxi::rutaxi::Object to;
  taxi::rutaxi::MakeNearObject(kNearObject, to);
  taxi::rutaxi::City city = {"moscow", "RUB"};

  std::vector<taxi::Product> dst;
  taxi::rutaxi::MakeProducts(src, from, to, city, dst);
  TEST_EQUAL(dst.size(), 1, ());
  TEST_EQUAL(dst[0].m_price, "1005", ());
  TEST_EQUAL(dst[0].m_currency, "RUB", ());

  TEST(!dst[0].m_productId.empty(), ());
  TEST(!dst[0].m_time.empty(), ());
  TEST(dst[0].m_name.empty(), ());
}

UNIT_TEST(RuTaxi_LoadCityMapping)
{
  auto const mapping = taxi::rutaxi::LoadCityMapping();
  TEST(!mapping.empty(), ());

  for (auto const & item : mapping)
  {
    TEST(!item.second.m_id.empty(), ());
    TEST(!item.second.m_currency.empty(), ());
  }
}
}  // namespace
