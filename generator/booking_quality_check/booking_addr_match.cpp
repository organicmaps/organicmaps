#include "generator/booking_dataset.hpp"
#include "generator/osm_source.hpp"

#include "map/framework.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "search/processor_factory.hpp"
#include "search/ranking_info.hpp"
#include "search/result.hpp"
#include "search/search_quality/helpers.hpp"
#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/index.hpp"
#include "storage/storage.hpp"

#include "coding/reader_wrapper.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/numeric.hpp"
#include "std/shared_ptr.hpp"

#include "3party/gflags/src/gflags/gflags.h"

using namespace generator;
using namespace storage;
using namespace search;
using namespace search::tests_support;

void DidDownload(TCountryId const & /* countryId */,
                 shared_ptr<platform::LocalCountryFile> const & /* localFile */)
{
}

bool WillDelete(TCountryId const & /* countryId */,
                shared_ptr<platform::LocalCountryFile> const & /* localFile */)
{
  return false;
}

uint64_t ReadVersionFromHeader(platform::LocalCountryFile const & mwm)
{
  vector<string> specialFiles = {
    WORLD_FILE_NAME,
    WORLD_COASTS_FILE_NAME,
    WORLD_COASTS_OBSOLETE_FILE_NAME
  };
  for (auto const & name : specialFiles)
  {
    if (mwm.GetCountryName() == name)
      return mwm.GetVersion();
  }

  ModelReaderPtr reader = FilesContainerR(mwm.GetPath(MapOptions::Map)).GetReader(VERSION_FILE_TAG);
  ReaderSrc src(reader.GetPtr());

  version::MwmVersion version;
  version::ReadVersion(src, version);
  return version.GetVersion();
}

DEFINE_string(booking_data, "", "Path to booking data in .tsv format");
DEFINE_string(user_resource_path, "", "Path to data directory (resources dir)");
DEFINE_string(data_path, "", "Path to mwm files (writable dir)");
DEFINE_string(locale, "en", "Locale of all the search queries");
DEFINE_int32(num_threads, 1, "Number of search engine threads");


int main(int argc, char * argv[])
{
  google::SetUsageMessage("Takes OSM XML data from stdin and creates"
                          " data and index files in several passes.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  Platform & platform = GetPlatform();

  string countriesFile = COUNTRIES_FILE;
  if (!FLAGS_user_resource_path.empty())
  {
    platform.SetResourceDir(FLAGS_user_resource_path);
    countriesFile = my::JoinFoldersToPath(FLAGS_user_resource_path, COUNTRIES_FILE);
  }

  if (!FLAGS_data_path.empty())
  {
    platform.SetSettingsDirForTests(FLAGS_data_path);
    platform.SetWritableDirForTests(FLAGS_data_path);
  }

  LOG(LINFO, ("writable dir =", platform.WritableDir()));
  LOG(LINFO, ("resources dir =", platform.ResourcesDir()));

  LOG_SHORT(LINFO, ("Booking data:", FLAGS_booking_data));

  BookingDataset bookingDataset(FLAGS_booking_data);


  Framework f;

  size_t matchedNum = 0;
  size_t emptyAddr = 0;
  for (size_t i = 0; i < bookingDataset.Size(); ++i)
  {
    BookingDataset::Hotel const & hotel = bookingDataset.GetHotel(i);

    search::AddressInfo info = f.GetAddressInfoAtPoint(MercatorBounds::FromLatLon(hotel.lat, hotel.lon));

    if (hotel.address.empty())
      ++emptyAddr;

    if(!info.FormatAddress().empty())
    {
      ++matchedNum;
      cout << "[" << i << "/" << bookingDataset.Size() << "] Hotel: " << hotel.address
      << " AddLoc: " << hotel.addressLoc << " --> " << info.FormatAddress() << endl;
    }
  }

  cout << "Num of hotels: " << bookingDataset.Size() << " matched: " << matchedNum << " Empty addresses: " << emptyAddr << endl;

  return 0;
}
