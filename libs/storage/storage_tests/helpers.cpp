#include "testing/testing.hpp"

#include "storage/storage_tests/helpers.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

#include <memory>

namespace storage
{
std::unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter()
{
  return CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
}
}  // namespace storage
