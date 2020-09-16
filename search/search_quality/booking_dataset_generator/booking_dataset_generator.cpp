#include "search/search_quality/helpers.hpp"
#include "search/search_quality/sample.hpp"

#include "search/result.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"
#include "indexer/utils.hpp"

#include "platform/platform_tests_support/helpers.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "3party/gflags/src/gflags/gflags.h"

using namespace search::search_quality;
using namespace search;
using namespace std;
using namespace storage;

DEFINE_string(data_path, "", "Path to data directory (resources dir).");
DEFINE_string(mwm_path, "", "Path to mwm files (writable dir).");
DEFINE_string(out_path, "samples.jsonl", "Path to output samples file.");
DEFINE_string(dataset_type, "name",
              "Dataset type: name (search hotel by name) or address (search hotel by address).");
DEFINE_string(address_dataset_path, "", "Path to address dataset.");

string GetSampleString(FeatureType & hotel, m2::PointD const & userPos, string const & address)
{
  Sample sample;
  string hotelName;
  double constexpr kViewportRadiusM = 1000.0;
  if (!address.empty())
  {
    sample.m_query = strings::MakeUniString(address + " ");
  }
  else
  {
    if (!hotel.GetName(StringUtf8Multilang::kEnglishCode, hotelName) &&
        !hotel.GetName(StringUtf8Multilang::kDefaultCode, hotelName))
    {
      LOG(LINFO, ("Cannot get name for", hotel.GetID()));
      return "";
    }
    sample.m_query = strings::MakeUniString(hotelName + " ");
  }
  sample.m_locale = "en";
  sample.m_pos = userPos;
  sample.m_viewport = mercator::RectByCenterXYAndSizeInMeters(userPos, kViewportRadiusM);
  sample.m_results.push_back(Sample::Result::Build(hotel, Sample::Result::Relevance::Vital));
  string json;
  Sample::SerializeToJSONLines({sample}, json);
  return json;
}

enum class Fields : uint8_t
{
  SponsoredId = 0,
  Address = 1,
  Zip = 2,
  City = 3,
  District = 4,
  Country = 5,
  Count = 6
};

string CreateAddress(vector<string> const & fields)
{
  string result = fields[base::Underlying(Fields::Address)];
  if (result.empty())
    return {};

  auto const district = fields[base::Underlying(Fields::District)];
  if (district != "None")
    result += ", " + district;
  result += ", " + fields[base::Underlying(Fields::Zip)];
  result += ", " + fields[base::Underlying(Fields::City)];
  result += ", " + fields[base::Underlying(Fields::Country)];
  return result;
}

map<string, string> ParseAddressDataset(string const & filename)
{
  if (filename.empty())
    return {};

  map<string, string> result;
  ifstream data(filename);
  string line;
  // Skip header.
  getline(data, line);
  while (getline(data, line))
  {
    vector<string> fields;
    strings::ParseCSVRow(line, '\t', fields);
    CHECK_EQUAL(fields.size(), base::Underlying(Fields::Count), ());
    auto const id = fields[base::Underlying(Fields::SponsoredId)];
    auto const address = CreateAddress(fields);
    if (address.empty())
      continue;
    auto const ret = result.emplace(id, address);
    // Hotel may appear several times.
    if (!ret.second)
      CHECK_EQUAL(result[id], address, ());
  }
  return result;
}

int main(int argc, char * argv[])
{
  platform::tests_support::ChangeMaxNumberOfOpenFiles(kMaxOpenFiles);
  CheckLocale();

  google::SetUsageMessage("Booking dataset generator.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_dataset_type != "name" && FLAGS_dataset_type != "address")
  {
    LOG(LERROR, ("Wrong dataset type:", FLAGS_dataset_type, ". Supported types: name, address"));
    return -1;
  }

  auto const generateAddress = FLAGS_dataset_type == "address";

  if (generateAddress && FLAGS_address_dataset_path.empty())
  {
    LOG(LERROR, ("Set address_dataset_path."));
    return -1;
  }

  SetPlatformDirs(FLAGS_data_path, FLAGS_mwm_path);

  classificator::Load();

  FrozenDataSource dataSource;
  InitDataSource(dataSource, "" /* mwmListPath */);

  ofstream out;
  out.open(FLAGS_out_path);
  if (!out.is_open())
  {
    LOG(LERROR, ("Can't open output file", FLAGS_out_path));
    return -1;
  }

  auto const & hotelChecker = ftypes::IsBookingHotelChecker::Instance();

  map<string, string> addressData;
  if (generateAddress)
  {
    addressData = ParseAddressDataset(FLAGS_address_dataset_path);
  }

  auto const getAddress = [&](FeatureType & hotel) -> string {
    auto const id = hotel.GetMetadata(feature::Metadata::FMD_SPONSORED_ID);
    if (id.empty())
      return {};

    auto const it = addressData.find(id);
    if (it == addressData.end())
      return {};

    return it->second;
  };

  // For all airports from World.mwm (international or other important airports) and all
  // hotels which are closer than 100 km from airport we create sample with query=|hotel name| and
  // viewport and position in the airport.
  double constexpr kDistanceToHotelM = 1e5;
  set<FeatureID> hotelsNextToAirport;
  {
    auto const handle = indexer::FindWorld(dataSource);
    if (!handle.IsAlive())
    {
      LOG(LERROR, ("Cannot find World.mwm"));
      return -1;
    }

    auto const & airportChecker = ftypes::IsAirportChecker::Instance();
    FeaturesLoaderGuard const guard(dataSource, handle.GetId());
    for (uint32_t i = 0; i < guard.GetNumFeatures(); ++i)
    {
      auto airport = guard.GetFeatureByIndex(i);
      if (!airportChecker(*airport))
        continue;

      auto const airportPos = feature::GetCenter(*airport);
      auto addHotel = [&](FeatureType & hotel) {
        if (!hotelChecker(hotel))
          return;

        if (mercator::DistanceOnEarth(airportPos, feature::GetCenter(hotel)) >
            kDistanceToHotelM)
        {
          return;
        }

        string address;
        if (generateAddress)
        {
          address = getAddress(hotel);
          if (address.empty())
            return;
        }

        string const json = GetSampleString(hotel, airportPos, address);
        if (json.empty())
          return;
        out << json;
        hotelsNextToAirport.insert(hotel.GetID());
      };

      dataSource.ForEachInRect(
          addHotel, mercator::RectByCenterXYAndSizeInMeters(airportPos, kDistanceToHotelM),
          scales::GetUpperScale());
    }
    LOG(LINFO, (hotelsNextToAirport.size(), "hotels have nearby airport."));
  }

  // For all hotels without an airport nearby we set user position 100km away from hotel.
  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);
  for (auto const & info : infos)
  {
    auto handle = dataSource.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
    {
      LOG(LERROR, ("Mwm reading error", info));
      return -1;
    }
    FeaturesLoaderGuard const guard(dataSource, handle.GetId());
    for (uint32_t i = 0; i < guard.GetNumFeatures(); ++i)
    {
      auto hotel = guard.GetFeatureByIndex(i);
      if (!hotelChecker(*hotel))
        continue;

      if (hotelsNextToAirport.count(hotel->GetID()) != 0)
        continue;

      string address;
      if (generateAddress)
      {
        address = getAddress(*hotel);
        if (address.empty())
          continue;
      }

      static double kRadiusToHotelM = kDistanceToHotelM / sqrt(2.0);
      string json = GetSampleString(
          *hotel,
          mercator::GetSmPoint(feature::GetCenter(*hotel), kRadiusToHotelM, kRadiusToHotelM),
          address);

      if (!json.empty())
        out << json;
    }
  }

  return 0;
}
