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
} // namespace storage
