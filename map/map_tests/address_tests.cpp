#include "testing/testing.hpp"

#include "search/reverse_geocoder.hpp"
#include "search/search_string_utils.hpp"

#include "indexer/index.hpp"


namespace
{

void TestAddress(search::ReverseGeocoder & coder,
                 ms::LatLon const & ll, string const & stName, string const & hName)
{
  search::ReverseGeocoder::Street street;
  search::ReverseGeocoder::Building building;
  coder.GetNearbyAddress(MercatorBounds::FromLatLon(ll), building, street);

  string key;
  search::GetStreetNameAsKey(street.m_name, key);

  TEST_EQUAL(stName, key, ());
  TEST_EQUAL(hName, building.m_name, ());
}

} // namespace

UNIT_TEST(ReverseGeocoder_Smoke)
{
  platform::LocalCountryFile file = platform::LocalCountryFile::MakeForTesting("minsk-pass");

  Index index;
  TEST_EQUAL(index.RegisterMap(file).second, MwmSet::RegResult::Success, ());

  search::ReverseGeocoder coder(index);

  TestAddress(coder, {53.89815, 27.54265}, "мясникова", "32");
  TestAddress(coder, {53.89953, 27.54189}, "немига", "42");
  TestAddress(coder, {53.89666, 27.54904}, "советская", "19");
  TestAddress(coder, {53.89724, 27.54983}, "независимости", "11");
  TestAddress(coder, {53.89745, 27.55835}, "карламаркса", "18А");
}
