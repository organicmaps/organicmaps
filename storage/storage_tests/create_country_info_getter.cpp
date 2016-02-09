#include "testing/testing.hpp"

#include "storage/storage_tests/create_country_info_getter.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

namespace storage
{
unique_ptr<CountryInfoGetter> CreateCountryInfoGetter()
{
  Platform & platform = GetPlatform();
  return unique_ptr<CountryInfoGetter>(new CountryInfoReader(platform.GetReader(PACKED_POLYGONS_FILE),
                                                             platform.GetReader(COUNTRIES_FILE)));
}

unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetterMigrate()
{
  Platform & platform = GetPlatform();
  return unique_ptr<CountryInfoGetter>(new CountryInfoReader(platform.GetReader(PACKED_POLYGONS_MIGRATE_FILE),
                                                             platform.GetReader(COUNTRIES_MIGRATE_FILE)));
}

void TestAlmostEqualRectsAbs(const m2::RectD & r1, const m2::RectD & r2)
{
  double constexpr kEpsilon = 1e-3;
  TEST(my::AlmostEqualAbs(r1.maxX(), r2.maxX(), kEpsilon), ());
  TEST(my::AlmostEqualAbs(r1.maxY(), r2.maxY(), kEpsilon), ());
  TEST(my::AlmostEqualAbs(r1.minX(), r2.minX(), kEpsilon), ());
  TEST(my::AlmostEqualAbs(r1.minY(), r2.minY(), kEpsilon), ());
}
} // namespace storage
