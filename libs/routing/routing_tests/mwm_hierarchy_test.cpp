#include "testing/testing.hpp"

#include "routing/mwm_hierarchy_handler.hpp"

#include "storage/country_parent_getter.hpp"
#include "storage/routing_helpers.hpp"

namespace mwm_hierarchy_test
{
using namespace routing;

UNIT_TEST(CountryParentGetter_Smoke)
{
  storage::CountryParentGetter getter;
  TEST_EQUAL(getter("Belarus_Hrodna Region"), "Belarus", ());
  TEST_EQUAL(getter("Russia_Arkhangelsk Oblast_Central"), "Russian Federation", ());
  TEST_EQUAL(getter("Crimea"), "", ());

  TEST_EQUAL(getter("Israel"), "Israel Region", ());
  TEST_EQUAL(getter("Palestine"), "Palestine Region", ());
  TEST_EQUAL(getter("Jerusalem"), "", ());
  TEST_EQUAL(getter("US_New York_New York"), "New York", ());
  TEST_EQUAL(getter("UK_England_West Midlands"), "United Kingdom", ());
}

uint16_t GetCountryID(std::shared_ptr<NumMwmIds> const & mwmIDs, std::string mwmName)
{
  return mwmIDs->GetId(platform::CountryFile(std::move(mwmName)));
}

UNIT_TEST(MwmHierarchyHandler_Smoke)
{
  storage::CountryParentGetter getter;
  auto mwmIDs = CreateNumMwmIds(getter.GetStorageForTesting());
  routing::MwmHierarchyHandler handler(mwmIDs, getter);

  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Belarus_Vitebsk Region")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                     GetCountryID(mwmIDs, "Lithuania_East")),
       ());
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Russia_Smolensk Oblast")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ukraine_Kherson Oblast"), GetCountryID(mwmIDs, "Crimea")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Russia_Krasnodar Krai"), GetCountryID(mwmIDs, "Crimea")),
       ());
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Denmark_Region Zealand"),
                                      GetCountryID(mwmIDs, "Denmark_Region of Southern Denmark")),
       ());

  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ukraine_Zakarpattia Oblast"),
                                     GetCountryID(mwmIDs, "Slovakia_Region of Kosice")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ukraine_Zakarpattia Oblast"),
                                     GetCountryID(mwmIDs, "Hungary_Northern Great Plain")),
       ());
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Hungary_Northern Great Plain"),
                                      GetCountryID(mwmIDs, "Slovakia_Region of Kosice")),
       ());

  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ireland_Connacht"),
                                      GetCountryID(mwmIDs, "UK_Northern Ireland")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ireland_Leinster"), GetCountryID(mwmIDs, "UK_Wales")), ());

  char const * ip[] = {"Israel", "Jerusalem", "Palestine"};
  for (auto s1 : ip)
    for (auto s2 : ip)
      TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, s1), GetCountryID(mwmIDs, s2)), (s1, s2));
}

}  // namespace mwm_hierarchy_test
