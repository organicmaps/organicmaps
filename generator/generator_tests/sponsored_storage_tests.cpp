#include "testing/testing.hpp"

#include "generator/booking_dataset.hpp"
#include "generator/sponsored_object_storage.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_name_utils.hpp"

using platform::tests_support::ScopedFile;

double const kDummyDistanseForTesting = 1.0;
size_t const kDummyCountOfObjectsForTesting = 1;
std::string const kExcludedContent = "100\n200\n300";
std::string const kExcludedIdsFileName = "excluded_for_testing.txt";

namespace
{
UNIT_TEST(LoadExcludedIds)
{
  ScopedFile sf(kExcludedIdsFileName, kExcludedContent);

  generator::SponsoredObjectStorage<generator::BookingHotel> storage(
      kDummyDistanseForTesting, kDummyCountOfObjectsForTesting);

  auto const & path = my::JoinPath(GetPlatform().WritableDir(), kExcludedIdsFileName);
  auto const excludedIds = storage.LoadExcludedIds(path);
  generator::BookingHotel::ObjectId id;
  TEST_EQUAL(excludedIds.size(), 3, ());
  id.Set(100);
  TEST(excludedIds.find(id) != excludedIds.cend(), ());
  id.Set(200);
  TEST(excludedIds.find(id) != excludedIds.cend(), ());
  id.Set(300);
  TEST(excludedIds.find(id) != excludedIds.cend(), ());
}
}  // namespace
