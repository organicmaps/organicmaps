#include "testing/testing.hpp"

#include "indexer/postcodes_matcher.hpp"

namespace postcodes_matcher_test
{
using namespace search;

UNIT_TEST(PostcodesMatcher_Smoke)
{
  /// @todo We are not _complete_ here, because G4 is also a postcode prefix (Glasgow, Scotland)
  /// like BN1 or BN3 (Brighton).
  TEST(!LooksLikePostcode("G4", false /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("G4", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BN1", false /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BN3 ", false /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BN1", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BN3 ", true /* handleAsPrefix */), ());

  TEST(LooksLikePostcode("141701", false /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("141", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BA6 8JP", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BA6-8JP", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BA22 9HR", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("BA22", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("DE56 4FW", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("NY 1000", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("AZ 85203", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("AZ", true /* handleAsPrefix */), ());

  TEST(LooksLikePostcode("803 0271", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("803-0271", true /* handleAsPrefix */), ());
  TEST(LooksLikePostcode("〒803-0271", true /* handleAsPrefix */), ());

  TEST(!LooksLikePostcode("1 мая", true /* handleAsPrefix */), ());
  TEST(!LooksLikePostcode("1 мая улица", true /* handleAsPrefix */), ());
  TEST(!LooksLikePostcode("москва", true /* handleAsPrefix */), ());
  TEST(!LooksLikePostcode("39 с 79", true /* handleAsPrefix */), ());
}

}  // namespace postcodes_matcher_test
