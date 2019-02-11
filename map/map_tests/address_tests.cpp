#include "testing/testing.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/preferred_languages.hpp"

using namespace search;
using namespace platform;

namespace
{
void TestAddress(ReverseGeocoder & coder, ms::LatLon const & ll, string const & street,
                 string const & houseNumber)
{
  ReverseGeocoder::Address addr;
  coder.GetNearbyAddress(MercatorBounds::FromLatLon(ll), addr);

  string const key =
      strings::ToUtf8(GetStreetNameAsKey(addr.m_street.m_name, false /* ignoreStreetSynonyms */));

  TEST_EQUAL(key, street, (addr));
  TEST_EQUAL(houseNumber, addr.GetHouseNumber(), (addr));
}
} // namespace

UNIT_TEST(ReverseGeocoder_Smoke)
{
  classificator::Load();

  LocalCountryFile file = LocalCountryFile::MakeForTesting("minsk-pass");

  FrozenDataSource dataSource;
  TEST_EQUAL(dataSource.RegisterMap(file).second, MwmSet::RegResult::Success, ());

  ReverseGeocoder coder(dataSource);

  TEST_EQUAL(languages::GetCurrentNorm(), "en", ());

  TestAddress(coder, {53.89815, 27.54265}, "vulicamiasnikova", "32");
  TestAddress(coder, {53.89953, 27.54189}, "vulicaniamiha", "42");
  TestAddress(coder, {53.89666, 27.54904}, "savieckajavulica", "19");
  TestAddress(coder, {53.89724, 27.54983}, "praspiektniezalieznasci", "11");
  TestAddress(coder, {53.89745, 27.55835}, "vulicakarlamarksa", "18А");
}
