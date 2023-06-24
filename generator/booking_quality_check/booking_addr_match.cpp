#include "generator/booking_dataset.hpp"
#include "generator/utils.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/data_source.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include <iostream>

#include <gflags/gflags.h>


DEFINE_string(booking_data, "", "Path to booking data in .tsv format");
DEFINE_string(user_resource_path, "", "Path to data directory (resources dir)");
DEFINE_string(data_path, "", "Path to mwm files (writable dir)");
DEFINE_string(locale, "en", "Locale of all the search queries");
DEFINE_int32(num_threads, 1, "Number of search engine threads");

namespace
{

class AddressMatcher
{
public:
  AddressMatcher()
  {
    LoadDataSource(m_dataSource);
    m_coder = std::make_unique<search::ReverseGeocoder>(m_dataSource);
  }

  template <typename SponsoredObject>
  void operator()(SponsoredObject & object)
  {
    search::ReverseGeocoder::Address addr;
    m_coder->GetNearbyAddress(mercator::FromLatLon(object.m_latLon), addr);
    object.m_street = addr.GetStreetName();
    object.m_houseNumber = addr.GetHouseNumber();
  }

private:
  FrozenDataSource m_dataSource;
  std::unique_ptr<search::ReverseGeocoder> m_coder;
};

} // namespace

int main(int argc, char * argv[])
{
  gflags::SetUsageMessage(
      "Takes OSM XML data from stdin and creates"
      " data and index files in several passes.");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  Platform & platform = GetPlatform();

  if (!FLAGS_user_resource_path.empty())
    platform.SetResourceDir(FLAGS_user_resource_path);

  if (!FLAGS_data_path.empty())
    platform.SetWritableDirForTests(FLAGS_data_path);

  LOG(LINFO, ("writable dir =", platform.WritableDir()));
  LOG(LINFO, ("resources dir =", platform.ResourcesDir()));

  LOG_SHORT(LINFO, ("Booking data:", FLAGS_booking_data));

  generator::BookingDataset bookingDataset(FLAGS_booking_data);
  AddressMatcher addressMatcher;

  size_t matchedNum = 0;
  size_t emptyAddr = 0;
  auto const & storage = bookingDataset.GetStorage();
  for (auto [_, hotel] : storage.GetObjects())
  {
    addressMatcher(hotel);

    if (hotel.m_address.empty())
      ++emptyAddr;

    if (hotel.HasAddresParts())
    {
      ++matchedNum;
      std::cout << "Hotel: " << hotel.m_address << " AddLoc: " << hotel.m_translations << " --> "
                << hotel.m_street << " " << hotel.m_houseNumber << std::endl;
    }
  }

  std::cout << "Num of hotels: " << storage.Size() << " matched: " << matchedNum
            << " Empty addresses: " << emptyAddr << std::endl;

  return 0;
}
