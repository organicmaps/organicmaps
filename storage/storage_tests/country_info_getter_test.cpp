#include "testing/testing.hpp"

#include "storage/storage_tests/helpers.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/country.hpp"
#include "storage/storage.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"


using namespace storage;

namespace
{
bool IsEmptyName(map<string, CountryInfo> const & id2info, string const & id)
{
  auto const it = id2info.find(id);
  TEST(it != id2info.end(), ());
  return it->second.m_name.empty();
}
}  // namespace

UNIT_TEST(CountryInfoGetter_GetByPoint_Smoke)
{
  auto const getter = CreateCountryInfoGetter();

  CountryInfo info;

  // Minsk
  getter->GetRegionInfo(MercatorBounds::FromLatLon(53.9022651, 27.5618818), info);
  TEST_EQUAL(info.m_name, "Belarus", ());

  getter->GetRegionInfo(MercatorBounds::FromLatLon(-6.4146288, -38.0098101), info);
  TEST_EQUAL(info.m_name, "Brazil, Northeast", ());

  getter->GetRegionInfo(MercatorBounds::FromLatLon(34.6509, 135.5018), info);
  TEST_EQUAL(info.m_name, "Japan, Kinki", ());
}

UNIT_TEST(CountryInfoGetter_GetRegionsCountryIdByRect_Smoke)
{
  auto const getter = CreateCountryInfoGetter();

  m2::PointD const p = MercatorBounds::FromLatLon(53.9022651, 27.5618818);

  // Inside mwm.
  m2::PointD const halfSize = m2::PointD(0.1, 0.1);
  auto const countries =
      getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize, p + halfSize), false /* rough */);
  TEST_EQUAL(countries, vector<storage::TCountryId>{"Belarus"}, ());

  // Several countries.
  m2::PointD const halfSize2 = m2::PointD(5.0, 5.0);
  auto const countries2 = getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize2, p + halfSize2),
                                                            false /* rough */);
  auto const expected = vector<storage::TCountryId>{
      "Belarus", "Latvia", "Lithuania", "Poland", "Russia_Central", "Russia_Northwestern",
      "Ukraine"};
  TEST_EQUAL(countries2, expected, ());

  // No one found.
  auto const countries3 =
      getter->GetRegionsCountryIdByRect(m2::RectD(-halfSize, halfSize), false /* rough */);
  TEST_EQUAL(countries3, vector<storage::TCountryId>{}, ());

  // Inside mwm (rough).
  auto const countries4 =
      getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize, p + halfSize), true /* rough */);
  TEST_EQUAL(countries, vector<storage::TCountryId>{"Belarus"}, ());

  // Several countries (rough).
  auto const countries5 =
      getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize2, p + halfSize2), true /* rough */);
  auto const expected2 = vector<storage::TCountryId>{"Belarus",
                                                     "Latvia",
                                                     "Lithuania",
                                                     "Poland",
                                                     "Russia_Central",
                                                     "Russia_Far Eastern",
                                                     "Russia_Northwestern",
                                                     "Sweden",
                                                     "Ukraine",
                                                     "USA_Alaska"};
  TEST_EQUAL(countries5, expected2, ());
}

UNIT_TEST(CountryInfoGetter_ValidName_Smoke)
{
  string buffer;
  ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(buffer);

  map<string, CountryInfo> id2info;
  bool isSingleMwm;
  storage::LoadCountryFile2CountryInfo(buffer, id2info, isSingleMwm);

  Storage storage;

  if (version::IsSingleMwm(storage.GetCurrentDataVersion()))
  {
    TEST(!IsEmptyName(id2info, "Belgium_West Flanders"), ());
    TEST(!IsEmptyName(id2info, "France_Ile-de-France_Paris"), ());
  }
  else
  {
    TEST(!IsEmptyName(id2info, "Germany_Baden-Wurttemberg"), ());
    TEST(!IsEmptyName(id2info, "France_Paris & Ile-de-France"), ());
  }
}

UNIT_TEST(CountryInfoGetter_SomeRects)
{
  auto const getter = CreateCountryInfoGetter();

  m2::RectD rects[3];
  getter->CalcUSALimitRect(rects);

  LOG(LINFO, ("USA Continental: ", rects[0]));
  LOG(LINFO, ("Alaska: ", rects[1]));
  LOG(LINFO, ("Hawaii: ", rects[2]));

  LOG(LINFO, ("Canada: ", getter->CalcLimitRect("Canada_")));
}

UNIT_TEST(CountryInfoGetter_HitsInRadius)
{
  auto const getter = CreateCountryInfoGetterMigrate();
  TCountriesVec results;
  getter->GetRegionsCountryId(MercatorBounds::FromLatLon(56.1702, 28.1505), results);
  TEST_EQUAL(results.size(), 3, ());
  TEST(find(results.begin(), results.end(), "Belarus_Vitebsk Region") != results.end(), ());
  TEST(find(results.begin(), results.end(), "Latvia") != results.end(), ());
  TEST(find(results.begin(), results.end(), "Russia_Pskov Oblast") != results.end(), ());
}

UNIT_TEST(CountryInfoGetter_HitsOnLongLine)
{
  auto const getter = CreateCountryInfoGetterMigrate();
  TCountriesVec results;
  getter->GetRegionsCountryId(MercatorBounds::FromLatLon(62.2507, -102.0753), results);
  TEST_EQUAL(results.size(), 2, ());
  TEST(find(results.begin(), results.end(), "Canada_Northwest Territories_East") != results.end(), ());
  TEST(find(results.begin(), results.end(), "Canada_Nunavut_South") != results.end(), ());
}

UNIT_TEST(CountryInfoGetter_HitsInTheMiddleOfNowhere)
{
  auto const getter = CreateCountryInfoGetterMigrate();
  TCountriesVec results;
  getter->GetRegionsCountryId(MercatorBounds::FromLatLon(62.2900, -103.9423), results);
  TEST_EQUAL(results.size(), 1, ());
  TEST(find(results.begin(), results.end(), "Canada_Northwest Territories_East") != results.end(), ());
}

UNIT_TEST(CountryInfoGetter_GetLimitRectForLeafSingleMwm)
{
  auto const getter = CreateCountryInfoGetterMigrate();
  Storage storage(COUNTRIES_FILE);
  if (!version::IsSingleMwm(storage.GetCurrentDataVersion()))
    return;

  m2::RectD const boundingBox = getter->GetLimitRectForLeaf("Angola");
  m2::RectD const expectedBoundingBox = {9.205259 /* minX */, -18.34456 /* minY */,
                                         24.08212 /* maxX */, -4.393187 /* maxY */};

  TEST(AlmostEqualRectsAbs(boundingBox, expectedBoundingBox), ());
}

UNIT_TEST(CountryInfoGetter_GetLimitRectForLeafTwoComponentMwm)
{
  auto const getter = CreateCountryInfoGetter();
  Storage storage(COUNTRIES_FILE);
  if (version::IsSingleMwm(storage.GetCurrentDataVersion()))
    return;

  m2::RectD const boundingBox = getter->GetLimitRectForLeaf("Angola");
  m2::RectD const expectedBoundingBox = {11.50151 /* minX */, -18.344569 /* minY */,
                                         24.08212 /* maxX */, -4.393187 /* maxY */};

  TEST(AlmostEqualRectsAbs(boundingBox, expectedBoundingBox), ());
}
