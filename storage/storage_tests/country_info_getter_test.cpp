#include "testing/testing.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/country.hpp"
#include "storage/storage.hpp"

#include "geometry/mercator.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/unique_ptr.hpp"


using namespace storage;

namespace
{
unique_ptr<CountryInfoGetter> CreateCountryInfoGetter()
{
  Platform & platform = GetPlatform();
  return make_unique<CountryInfoReader>(platform.GetReader(PACKED_POLYGONS_FILE),
                                        platform.GetReader(COUNTRIES_FILE));
}

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
