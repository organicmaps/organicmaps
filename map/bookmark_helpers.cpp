#include "map/bookmark_helpers.hpp"
#include "map/user.hpp"

#include "drape_frontend/visual_params.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_utils.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/sha1.hpp"
#include "coding/zip_reader.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include <map>
#include <sstream>

namespace
{
std::map<std::string, kml::BookmarkIcon> const kFeatureTypeToBookmarkIcon = {
  {"amenity-bbq", kml::BookmarkIcon::Food},
  {"tourism-picnic_site", kml::BookmarkIcon::Food},
  {"leisure-picnic_table", kml::BookmarkIcon::Food},
  {"amenity-cafe", kml::BookmarkIcon::Food},
  {"amenity-restaurant", kml::BookmarkIcon::Food},
  {"amenity-fast_food", kml::BookmarkIcon::Food},
  {"amenity-food_court", kml::BookmarkIcon::Food},
  {"amenity-bar", kml::BookmarkIcon::Food},
  {"amenity-pub", kml::BookmarkIcon::Food},
  {"amenity-biergarten", kml::BookmarkIcon::Food},

  {"waterway-waterfall", kml::BookmarkIcon::Sights},
  {"historic-tomb", kml::BookmarkIcon::Sights},
  {"historic-boundary_stone", kml::BookmarkIcon::Sights},
  {"historic-ship", kml::BookmarkIcon::Sights},
  {"historic-archaeological_site", kml::BookmarkIcon::Sights},
  {"historic-monument", kml::BookmarkIcon::Sights},
  {"historic-memorial", kml::BookmarkIcon::Sights},
  {"amenity-place_of_worship", kml::BookmarkIcon::Sights},
  {"tourism-attraction", kml::BookmarkIcon::Sights},
  {"tourism-theme_park", kml::BookmarkIcon::Sights},
  {"tourism-viewpoint", kml::BookmarkIcon::Sights},
  {"historic-fort", kml::BookmarkIcon::Sights},
  {"historic-castle", kml::BookmarkIcon::Sights},
  {"tourism-artwork", kml::BookmarkIcon::Sights},
  {"historic-ruins", kml::BookmarkIcon::Sights},
  {"historic-wayside_shrine", kml::BookmarkIcon::Sights},
  {"historic-wayside_cross", kml::BookmarkIcon::Sights},

  {"tourism-gallery", kml::BookmarkIcon::Museum},
  {"tourism-museum", kml::BookmarkIcon::Museum},
  {"amenity-arts_centre", kml::BookmarkIcon::Museum},

  {"sport", kml::BookmarkIcon::Entertainment},
  {"sport-multi", kml::BookmarkIcon::Entertainment},
  {"leisure-playground", kml::BookmarkIcon::Entertainment},
  {"leisure-water_park", kml::BookmarkIcon::Entertainment},
  {"amenity-casino", kml::BookmarkIcon::Entertainment},
  {"sport-archery", kml::BookmarkIcon::Entertainment},
  {"sport-shooting", kml::BookmarkIcon::Entertainment},
  {"sport-australian_football", kml::BookmarkIcon::Entertainment},
  {"sport-bowls", kml::BookmarkIcon::Entertainment},
  {"sport-curling", kml::BookmarkIcon::Entertainment},
  {"sport-cricket", kml::BookmarkIcon::Entertainment},
  {"sport-baseball", kml::BookmarkIcon::Entertainment},
  {"sport-basketball", kml::BookmarkIcon::Entertainment},
  {"sport-american_football", kml::BookmarkIcon::Entertainment},
  {"sport-athletics", kml::BookmarkIcon::Entertainment},
  {"sport-golf", kml::BookmarkIcon::Entertainment},
  {"sport-gymnastics", kml::BookmarkIcon::Entertainment},
  {"sport-tennis", kml::BookmarkIcon::Entertainment},
  {"sport-skiing", kml::BookmarkIcon::Entertainment},
  {"sport-soccer", kml::BookmarkIcon::Entertainment},
  {"amenity-nightclub", kml::BookmarkIcon::Entertainment},
  {"amenity-cinema", kml::BookmarkIcon::Entertainment},
  {"amenity-theatre", kml::BookmarkIcon::Entertainment},
  {"leisure-stadium", kml::BookmarkIcon::Entertainment},

  {"boundary-national_park", kml::BookmarkIcon::Park},
  {"leisure-nature_reserve", kml::BookmarkIcon::Park},
  {"landuse-forest", kml::BookmarkIcon::Park},

  {"natural-beach", kml::BookmarkIcon::Swim},
  {"sport-diving", kml::BookmarkIcon::Swim},
  {"sport-scuba_diving", kml::BookmarkIcon::Swim},
  {"sport-swimming", kml::BookmarkIcon::Swim},
  {"leisure-swimming_pool", kml::BookmarkIcon::Swim},

  {"natural-cave_entrance", kml::BookmarkIcon::Mountain},
  {"natural-peak", kml::BookmarkIcon::Mountain},
  {"natural-volcano", kml::BookmarkIcon::Mountain},
  {"natural-rock", kml::BookmarkIcon::Mountain},
  {"natural-bare_rock", kml::BookmarkIcon::Mountain},

  {"amenity-veterinary", kml::BookmarkIcon::Animals},
  {"leisure-dog_park", kml::BookmarkIcon::Animals},
  {"tourism-zoo", kml::BookmarkIcon::Animals},

  {"tourism-apartment", kml::BookmarkIcon::Hotel},
  {"tourism-chalet", kml::BookmarkIcon::Hotel},
  {"tourism-guest_house", kml::BookmarkIcon::Hotel},
  {"tourism-alpine_hut", kml::BookmarkIcon::Hotel},
  {"tourism-wilderness_hut", kml::BookmarkIcon::Hotel},
  {"tourism-hostel", kml::BookmarkIcon::Hotel},
  {"tourism-motel", kml::BookmarkIcon::Hotel},
  {"tourism-resort", kml::BookmarkIcon::Hotel},
  {"tourism-hotel", kml::BookmarkIcon::Hotel},
  {"sponsored-booking", kml::BookmarkIcon::Hotel},

  {"amenity-kindergarten", kml::BookmarkIcon::Building},
  {"amenity-school", kml::BookmarkIcon::Building},
  {"office", kml::BookmarkIcon::Building},
  {"amenity-library", kml::BookmarkIcon::Building},
  {"amenity-courthouse", kml::BookmarkIcon::Building},
  {"amenity-college", kml::BookmarkIcon::Building},
  {"amenity-police", kml::BookmarkIcon::Building},
  {"amenity-prison", kml::BookmarkIcon::Building},
  {"amenity-embassy", kml::BookmarkIcon::Building},
  {"office-lawyer", kml::BookmarkIcon::Building},
  {"building-train_station", kml::BookmarkIcon::Building},
  {"building-university", kml::BookmarkIcon::Building},

  {"amenity-atm", kml::BookmarkIcon::Exchange},
  {"amenity-bureau_de_change", kml::BookmarkIcon::Exchange},
  {"amenity-bank", kml::BookmarkIcon::Exchange},

  {"amenity-vending_machine", kml::BookmarkIcon::Shop},
  {"shop", kml::BookmarkIcon::Shop},
  {"amenity-marketplace", kml::BookmarkIcon::Shop},
  {"craft", kml::BookmarkIcon::Shop},
  {"shop-pawnbroker", kml::BookmarkIcon::Shop},
  {"shop-supermarket", kml::BookmarkIcon::Shop},
  {"shop-car_repair", kml::BookmarkIcon::Shop},
  {"shop-mall", kml::BookmarkIcon::Shop},
  {"highway-services", kml::BookmarkIcon::Shop},
  {"shop-stationery", kml::BookmarkIcon::Shop},
  {"shop-variety_store", kml::BookmarkIcon::Shop},
  {"shop-alcohol", kml::BookmarkIcon::Shop},
  {"shop-wine", kml::BookmarkIcon::Shop},
  {"shop-books", kml::BookmarkIcon::Shop},
  {"shop-bookmaker", kml::BookmarkIcon::Shop},
  {"shop-bakery", kml::BookmarkIcon::Shop},
  {"shop-beauty", kml::BookmarkIcon::Shop},
  {"shop-cosmetics", kml::BookmarkIcon::Shop},
  {"shop-beverages", kml::BookmarkIcon::Shop},
  {"shop-bicycle", kml::BookmarkIcon::Shop},
  {"shop-butcher", kml::BookmarkIcon::Shop},
  {"shop-car", kml::BookmarkIcon::Shop},
  {"shop-motorcycle", kml::BookmarkIcon::Shop},
  {"shop-car_parts", kml::BookmarkIcon::Shop},
  {"shop-car_repair", kml::BookmarkIcon::Shop},
  {"shop-tyres", kml::BookmarkIcon::Shop},
  {"shop-chemist", kml::BookmarkIcon::Shop},
  {"shop-clothes", kml::BookmarkIcon::Shop},
  {"shop-computer", kml::BookmarkIcon::Shop},
  {"shop-tattoo", kml::BookmarkIcon::Shop},
  {"shop-erotic", kml::BookmarkIcon::Shop},
  {"shop-funeral_directors", kml::BookmarkIcon::Shop},
  {"shop-confectionery", kml::BookmarkIcon::Shop},
  {"shop-chocolate", kml::BookmarkIcon::Shop},
  {"amenity-ice_cream", kml::BookmarkIcon::Shop},
  {"shop-convenience", kml::BookmarkIcon::Shop},
  {"shop-copyshop", kml::BookmarkIcon::Shop},
  {"shop-photo", kml::BookmarkIcon::Shop},
  {"shop-pet", kml::BookmarkIcon::Shop},
  {"shop-department_store", kml::BookmarkIcon::Shop},
  {"shop-doityourself", kml::BookmarkIcon::Shop},
  {"shop-electronics", kml::BookmarkIcon::Shop},
  {"shop-florist", kml::BookmarkIcon::Shop},
  {"shop-furniture", kml::BookmarkIcon::Shop},
  {"shop-garden_centre", kml::BookmarkIcon::Shop},
  {"shop-gift", kml::BookmarkIcon::Shop},
  {"shop-music", kml::BookmarkIcon::Shop},
  {"shop-video", kml::BookmarkIcon::Shop},
  {"shop-musical_instrument", kml::BookmarkIcon::Shop},
  {"shop-greengrocer", kml::BookmarkIcon::Shop},
  {"shop-hairdresser", kml::BookmarkIcon::Shop},
  {"shop-hardware", kml::BookmarkIcon::Shop},
  {"shop-jewelry", kml::BookmarkIcon::Shop},
  {"shop-kiosk", kml::BookmarkIcon::Shop},
  {"shop-laundry", kml::BookmarkIcon::Shop},
  {"shop-dry_cleaning", kml::BookmarkIcon::Shop},
  {"shop-mobile_phone", kml::BookmarkIcon::Shop},
  {"shop-optician", kml::BookmarkIcon::Shop},
  {"shop-outdoor", kml::BookmarkIcon::Shop},
  {"shop-seafood", kml::BookmarkIcon::Shop},
  {"shop-shoes", kml::BookmarkIcon::Shop},
  {"shop-sports", kml::BookmarkIcon::Shop},
  {"shop-ticket", kml::BookmarkIcon::Shop},
  {"shop-toys", kml::BookmarkIcon::Shop},
  {"shop-fabric", kml::BookmarkIcon::Shop},

  {"amenity-place_of_worship-christian", kml::BookmarkIcon::Christianity},
  {"landuse-cemetery-christian", kml::BookmarkIcon::Christianity},
  {"amenity-grave_yard-christian", kml::BookmarkIcon::Christianity},

  {"amenity-place_of_worship-jewish", kml::BookmarkIcon::Judaism},

  {"amenity-place_of_worship-buddhist", kml::BookmarkIcon::Buddhism},

  {"amenity-place_of_worship-muslim", kml::BookmarkIcon::Islam},

  {"amenity-parking", kml::BookmarkIcon::Parking},
  {"vending-parking_tickets", kml::BookmarkIcon::Parking},
  {"tourism-caravan_site", kml::BookmarkIcon::Parking},
  {"amenity-bicycle_parking", kml::BookmarkIcon::Parking},
  {"amenity-taxi", kml::BookmarkIcon::Parking},
  {"amenity-car_sharing", kml::BookmarkIcon::Parking},
  {"tourism-camp_site", kml::BookmarkIcon::Parking},
  {"amenity-motorcycle_parking", kml::BookmarkIcon::Parking},

  {"amenity-fuel", kml::BookmarkIcon::Gas},
  {"amenity-charging_station", kml::BookmarkIcon::Gas},

  {"amenity-fountain", kml::BookmarkIcon::Water},
  {"natural-spring", kml::BookmarkIcon::Water},
  {"amenity-drinking_water", kml::BookmarkIcon::Water},
  {"man_made-water_tap", kml::BookmarkIcon::Water},
  {"amenity-water_point", kml::BookmarkIcon::Water},

  {"amenity-dentist", kml::BookmarkIcon::Medicine},
  {"amenity-clinic", kml::BookmarkIcon::Medicine},
  {"amenity-doctors", kml::BookmarkIcon::Medicine},
  {"amenity-hospital", kml::BookmarkIcon::Medicine},
  {"amenity-pharmacy", kml::BookmarkIcon::Medicine},
  {"amenity-childcare", kml::BookmarkIcon::Medicine},
  {"emergency-defibrillator", kml::BookmarkIcon::Medicine}
};

std::map<std::string, BookmarkBaseType> const kFeatureTypeToBookmarkType = {
  {"amenity-bbq", BookmarkBaseType::Food},
  {"tourism-picnic_site", BookmarkBaseType::Food},
  {"leisure-picnic_table", BookmarkBaseType::Food},
  {"amenity-cafe", BookmarkBaseType::Food},
  {"amenity-restaurant", BookmarkBaseType::Food},
  {"amenity-fast_food", BookmarkBaseType::Food},
  {"amenity-food_court", BookmarkBaseType::Food},
  {"amenity-bar", BookmarkBaseType::Food},
  {"amenity-pub", BookmarkBaseType::Food},
  {"amenity-biergarten", BookmarkBaseType::Food},

  {"waterway-waterfall", BookmarkBaseType::Sights},
  {"historic-tomb", BookmarkBaseType::Sights},
  {"historic-boundary_stone", BookmarkBaseType::Sights},
  {"historic-ship", BookmarkBaseType::Sights},
  {"historic-archaeological_site", BookmarkBaseType::Sights},
  {"historic-monument", BookmarkBaseType::Sights},
  {"historic-memorial", BookmarkBaseType::Sights},
  {"tourism-attraction", BookmarkBaseType::Sights},
  {"tourism-theme_park", BookmarkBaseType::Sights},
  {"tourism-viewpoint", BookmarkBaseType::Sights},
  {"historic-fort", BookmarkBaseType::Sights},
  {"historic-castle", BookmarkBaseType::Sights},
  {"tourism-artwork", BookmarkBaseType::Sights},
  {"historic-ruins", BookmarkBaseType::Sights},
  {"historic-wayside_shrine", BookmarkBaseType::Sights},
  {"historic-wayside_cross", BookmarkBaseType::Sights},

  {"tourism-gallery", BookmarkBaseType::Museum},
  {"tourism-museum", BookmarkBaseType::Museum},
  {"amenity-arts_centre", BookmarkBaseType::Museum},

  {"sport", BookmarkBaseType::Entertainment},
  {"sport-multi", BookmarkBaseType::Entertainment},
  {"leisure-playground", BookmarkBaseType::Entertainment},
  {"leisure-water_park", BookmarkBaseType::Entertainment},
  {"amenity-casino", BookmarkBaseType::Entertainment},
  {"sport-archery", BookmarkBaseType::Entertainment},
  {"sport-shooting", BookmarkBaseType::Entertainment},
  {"sport-australian_football", BookmarkBaseType::Entertainment},
  {"sport-bowls", BookmarkBaseType::Entertainment},
  {"sport-curling", BookmarkBaseType::Entertainment},
  {"sport-cricket", BookmarkBaseType::Entertainment},
  {"sport-baseball", BookmarkBaseType::Entertainment},
  {"sport-basketball", BookmarkBaseType::Entertainment},
  {"sport-american_football", BookmarkBaseType::Entertainment},
  {"sport-athletics", BookmarkBaseType::Entertainment},
  {"sport-golf", BookmarkBaseType::Entertainment},
  {"sport-gymnastics", BookmarkBaseType::Entertainment},
  {"sport-tennis", BookmarkBaseType::Entertainment},
  {"sport-skiing", BookmarkBaseType::Entertainment},
  {"sport-soccer", BookmarkBaseType::Entertainment},
  {"amenity-nightclub", BookmarkBaseType::Entertainment},
  {"amenity-cinema", BookmarkBaseType::Entertainment},
  {"amenity-theatre", BookmarkBaseType::Entertainment},
  {"leisure-stadium", BookmarkBaseType::Entertainment},

  {"boundary-national_park", BookmarkBaseType::Park},
  {"leisure-nature_reserve", BookmarkBaseType::Park},
  {"landuse-forest", BookmarkBaseType::Park},

  {"natural-beach", BookmarkBaseType::Swim},
  {"sport-diving", BookmarkBaseType::Swim},
  {"sport-scuba_diving", BookmarkBaseType::Swim},
  {"sport-swimming", BookmarkBaseType::Swim},
  {"leisure-swimming_pool", BookmarkBaseType::Swim},

  {"natural-cave_entrance", BookmarkBaseType::Mountain},
  {"natural-peak", BookmarkBaseType::Mountain},
  {"natural-volcano", BookmarkBaseType::Mountain},
  {"natural-rock", BookmarkBaseType::Mountain},
  {"natural-bare_rock", BookmarkBaseType::Mountain},

  {"amenity-veterinary", BookmarkBaseType::Animals},
  {"leisure-dog_park", BookmarkBaseType::Animals},
  {"tourism-zoo", BookmarkBaseType::Animals},

  {"tourism-apartment", BookmarkBaseType::Hotel},
  {"tourism-chalet", BookmarkBaseType::Hotel},
  {"tourism-guest_house", BookmarkBaseType::Hotel},
  {"tourism-alpine_hut", BookmarkBaseType::Hotel},
  {"tourism-wilderness_hut", BookmarkBaseType::Hotel},
  {"tourism-hostel", BookmarkBaseType::Hotel},
  {"tourism-motel", BookmarkBaseType::Hotel},
  {"tourism-resort", BookmarkBaseType::Hotel},
  {"tourism-hotel", BookmarkBaseType::Hotel},
  {"sponsored-booking", BookmarkBaseType::Hotel},

  {"amenity-kindergarten", BookmarkBaseType::Building},
  {"amenity-school", BookmarkBaseType::Building},
  {"office", BookmarkBaseType::Building},
  {"amenity-library", BookmarkBaseType::Building},
  {"amenity-courthouse", BookmarkBaseType::Building},
  {"amenity-college", BookmarkBaseType::Building},
  {"amenity-police", BookmarkBaseType::Building},
  {"amenity-prison", BookmarkBaseType::Building},
  {"amenity-embassy", BookmarkBaseType::Building},
  {"office-lawyer", BookmarkBaseType::Building},
  {"building-train_station", BookmarkBaseType::Building},
  {"building-university", BookmarkBaseType::Building},

  {"amenity-atm", BookmarkBaseType::Exchange},
  {"amenity-bureau_de_change", BookmarkBaseType::Exchange},
  {"amenity-bank", BookmarkBaseType::Exchange},

  {"amenity-vending_machine", BookmarkBaseType::Shop},
  {"shop", BookmarkBaseType::Shop},
  {"amenity-marketplace", BookmarkBaseType::Shop},
  {"craft", BookmarkBaseType::Shop},
  {"shop-pawnbroker", BookmarkBaseType::Shop},
  {"shop-supermarket", BookmarkBaseType::Shop},
  {"shop-car_repair", BookmarkBaseType::Shop},
  {"shop-mall", BookmarkBaseType::Shop},
  {"highway-services", BookmarkBaseType::Shop},
  {"shop-stationery", BookmarkBaseType::Shop},
  {"shop-variety_store", BookmarkBaseType::Shop},
  {"shop-alcohol", BookmarkBaseType::Shop},
  {"shop-wine", BookmarkBaseType::Shop},
  {"shop-books", BookmarkBaseType::Shop},
  {"shop-bookmaker", BookmarkBaseType::Shop},
  {"shop-bakery", BookmarkBaseType::Shop},
  {"shop-beauty", BookmarkBaseType::Shop},
  {"shop-cosmetics", BookmarkBaseType::Shop},
  {"shop-beverages", BookmarkBaseType::Shop},
  {"shop-bicycle", BookmarkBaseType::Shop},
  {"shop-butcher", BookmarkBaseType::Shop},
  {"shop-car", BookmarkBaseType::Shop},
  {"shop-motorcycle", BookmarkBaseType::Shop},
  {"shop-car_parts", BookmarkBaseType::Shop},
  {"shop-car_repair", BookmarkBaseType::Shop},
  {"shop-tyres", BookmarkBaseType::Shop},
  {"shop-chemist", BookmarkBaseType::Shop},
  {"shop-clothes", BookmarkBaseType::Shop},
  {"shop-computer", BookmarkBaseType::Shop},
  {"shop-tattoo", BookmarkBaseType::Shop},
  {"shop-erotic", BookmarkBaseType::Shop},
  {"shop-funeral_directors", BookmarkBaseType::Shop},
  {"shop-confectionery", BookmarkBaseType::Shop},
  {"shop-chocolate", BookmarkBaseType::Shop},
  {"amenity-ice_cream", BookmarkBaseType::Shop},
  {"shop-convenience", BookmarkBaseType::Shop},
  {"shop-copyshop", BookmarkBaseType::Shop},
  {"shop-photo", BookmarkBaseType::Shop},
  {"shop-pet", BookmarkBaseType::Shop},
  {"shop-department_store", BookmarkBaseType::Shop},
  {"shop-doityourself", BookmarkBaseType::Shop},
  {"shop-electronics", BookmarkBaseType::Shop},
  {"shop-florist", BookmarkBaseType::Shop},
  {"shop-furniture", BookmarkBaseType::Shop},
  {"shop-garden_centre", BookmarkBaseType::Shop},
  {"shop-gift", BookmarkBaseType::Shop},
  {"shop-music", BookmarkBaseType::Shop},
  {"shop-video", BookmarkBaseType::Shop},
  {"shop-musical_instrument", BookmarkBaseType::Shop},
  {"shop-greengrocer", BookmarkBaseType::Shop},
  {"shop-hairdresser", BookmarkBaseType::Shop},
  {"shop-hardware", BookmarkBaseType::Shop},
  {"shop-jewelry", BookmarkBaseType::Shop},
  {"shop-kiosk", BookmarkBaseType::Shop},
  {"shop-laundry", BookmarkBaseType::Shop},
  {"shop-dry_cleaning", BookmarkBaseType::Shop},
  {"shop-mobile_phone", BookmarkBaseType::Shop},
  {"shop-optician", BookmarkBaseType::Shop},
  {"shop-outdoor", BookmarkBaseType::Shop},
  {"shop-seafood", BookmarkBaseType::Shop},
  {"shop-shoes", BookmarkBaseType::Shop},
  {"shop-sports", BookmarkBaseType::Shop},
  {"shop-ticket", BookmarkBaseType::Shop},
  {"shop-toys", BookmarkBaseType::Shop},
  {"shop-fabric", BookmarkBaseType::Shop},

  {"amenity-parking", BookmarkBaseType::Parking},
  {"vending-parking_tickets", BookmarkBaseType::Parking},
  {"tourism-caravan_site", BookmarkBaseType::Parking},
  {"amenity-bicycle_parking", BookmarkBaseType::Parking},
  {"amenity-taxi", BookmarkBaseType::Parking},
  {"amenity-car_sharing", BookmarkBaseType::Parking},
  {"tourism-camp_site", BookmarkBaseType::Parking},
  {"amenity-motorcycle_parking", BookmarkBaseType::Parking},

  {"amenity-place_of_worship", BookmarkBaseType::ReligiousPlace},
  {"amenity-place_of_worship-christian", BookmarkBaseType::ReligiousPlace},
  {"landuse-cemetery-christian", BookmarkBaseType::ReligiousPlace},
  {"amenity-grave_yard-christian", BookmarkBaseType::ReligiousPlace},
  {"amenity-place_of_worship-jewish", BookmarkBaseType::ReligiousPlace},
  {"amenity-place_of_worship-buddhist", BookmarkBaseType::ReligiousPlace},
  {"amenity-place_of_worship-muslim", BookmarkBaseType::ReligiousPlace},

  {"amenity-fuel", BookmarkBaseType::Gas},
  {"amenity-charging_station", BookmarkBaseType::Gas},

  {"amenity-fountain", BookmarkBaseType::Water},
  {"natural-spring", BookmarkBaseType::Water},
  {"amenity-drinking_water", BookmarkBaseType::Water},
  {"man_made-water_tap", BookmarkBaseType::Water},
  {"amenity-water_point", BookmarkBaseType::Water},

  {"amenity-dentist", BookmarkBaseType::Medicine},
  {"amenity-clinic", BookmarkBaseType::Medicine},
  {"amenity-doctors", BookmarkBaseType::Medicine},
  {"amenity-hospital", BookmarkBaseType::Medicine},
  {"amenity-pharmacy", BookmarkBaseType::Medicine},
  {"amenity-childcare", BookmarkBaseType::Medicine},
  {"emergency-defibrillator", BookmarkBaseType::Medicine}
};

void ValidateKmlData(std::unique_ptr<kml::FileData> & data)
{
  if (!data)
    return;

  for (auto & t : data->m_tracksData)
  {
    if (t.m_layers.empty())
      t.m_layers.emplace_back(kml::KmlParser::GetDefaultTrackLayer());
  }
}
}  // namespace

std::string const kKmzExtension = ".kmz";
std::string const kKmlExtension = ".kml";
std::string const kKmbExtension = ".kmb";

std::unique_ptr<kml::FileData> LoadKmlFile(std::string const & file, KmlFileType fileType)
{
  std::unique_ptr<kml::FileData> kmlData;
  try
  {
    kmlData = LoadKmlData(FileReader(file), fileType);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "loading failure:", e.what()));
    kmlData.reset();
  }
  if (kmlData == nullptr)
    LOG(LWARNING, ("Loading bookmarks failed, file", file));
  return kmlData;
}

std::unique_ptr<kml::FileData> LoadKmzFile(std::string const & file, std::string & kmlHash)
{
  std::string unarchievedPath;
  try
  {
    ZipFileReader::FileList files;
    ZipFileReader::FilesList(file, files);
    if (files.empty())
      return nullptr;

    auto fileName = files.front().first;
    for (auto const & f : files)
    {
      if (strings::MakeLowerCase(base::GetFileExtension(f.first)) == kKmlExtension)
      {
        fileName = f.first;
        break;
      }
    }
    unarchievedPath = file + ".raw";
    ZipFileReader::UnzipFile(file, fileName, unarchievedPath);
  }
  catch (ZipFileReader::OpenException const & ex)
  {
    LOG(LWARNING, ("Could not open zip file", ex.what()));
    return nullptr;
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("Unexpected exception on openning zip file", ex.what()));
    return nullptr;
  }

  if (!GetPlatform().IsFileExistsByFullPath(unarchievedPath))
    return nullptr;

  SCOPE_GUARD(fileGuard, std::bind(&FileWriter::DeleteFileX, unarchievedPath));

  kmlHash = coding::SHA1::CalculateBase64(unarchievedPath);
  return LoadKmlFile(unarchievedPath, KmlFileType::Text);
}

std::unique_ptr<kml::FileData> LoadKmlData(Reader const & reader, KmlFileType fileType)
{
  auto data = std::make_unique<kml::FileData>();
  try
  {
    if (fileType == KmlFileType::Binary)
    {
      kml::binary::DeserializerKml des(*data);
      des.Deserialize(reader);
    }
    else if (fileType == KmlFileType::Text)
    {
      kml::DeserializerKml des(*data);
      des.Deserialize(reader);
    }
    else
    {
      CHECK(false, ("Not supported KmlFileType"));
    }
    ValidateKmlData(data);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "reading failure:", e.what()));
    return nullptr;
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & e)
  {
    LOG(LWARNING, ("KML", fileType, "deserialization failure:", e.what()));
    return nullptr;
  }
  catch (kml::DeserializerKml::DeserializeException const & e)
  {
    LOG(LWARNING, ("KML", fileType, "deserialization failure:", e.what()));
    return nullptr;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "loading failure:", e.what()));
    return nullptr;
  }
  return data;
}

bool SaveKmlFile(kml::FileData & kmlData, std::string const & file, KmlFileType fileType)
{
  bool success;
  try
  {
    FileWriter writer(file);
    success = SaveKmlData(kmlData, writer, fileType);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "saving failure:", e.what()));
    success = false;
  }
  if (!success)
    LOG(LWARNING, ("Saving bookmarks failed, file", file));
  return success;
}

bool SaveKmlData(kml::FileData & kmlData, Writer & writer, KmlFileType fileType)
{
  try
  {
    if (fileType == KmlFileType::Binary)
    {
      kml::binary::SerializerKml ser(kmlData);
      ser.Serialize(writer);
    }
    else if (fileType == KmlFileType::Text)
    {
      kml::SerializerKml ser(kmlData);
      ser.Serialize(writer);
    }
    else
    {
      CHECK(false, ("Not supported KmlFileType"));
    }
  }
  catch (Writer::Exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "writing failure:", e.what()));
    return false;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "serialization failure:", e.what()));
    return false;
  }
  return true;
}

void ResetIds(kml::FileData & kmlData)
{
  kmlData.m_categoryData.m_id = kml::kInvalidMarkGroupId;
  for (auto & bmData : kmlData.m_bookmarksData)
    bmData.m_id = kml::kInvalidMarkId;
  for (auto & trackData : kmlData.m_tracksData)
    trackData.m_id = kml::kInvalidTrackId;
}

bool TruncType(std::string & type)
{
  static std::string const kDelim = "-";
  auto const pos = type.rfind(kDelim);
  if (pos == std::string::npos)
    return false;
  type.resize(pos);
  return true;
}

BookmarkBaseType GetBookmarkBaseType(std::vector<uint32_t> const & featureTypes)
{
  auto const & c = classif();
  for (auto typeIndex : featureTypes)
  {
    auto const type = c.GetTypeForIndex(typeIndex);
    auto typeStr = c.GetReadableObjectName(type);

    do
    {
      auto const itType = kFeatureTypeToBookmarkType.find(typeStr);
      if (itType != kFeatureTypeToBookmarkType.cend())
        return itType->second;
    } while (TruncType(typeStr));
  }
  return BookmarkBaseType::None;
}

kml::BookmarkIcon GetBookmarkIconByFeatureType(uint32_t type)
{
  auto typeStr = classif().GetReadableObjectName(type);

  do
  {
    auto const itIcon = kFeatureTypeToBookmarkIcon.find(typeStr);
    if (itIcon != kFeatureTypeToBookmarkIcon.cend())
      return itIcon->second;
  } while (TruncType(typeStr));

  return kml::BookmarkIcon::None;
}

void SaveFeatureTypes(feature::TypesHolder const & types, kml::BookmarkData & bmData)
{
  auto const & c = classif();
  feature::TypesHolder copy(types);
  copy.SortBySpec();
  bmData.m_featureTypes.reserve(copy.Size());
  for (auto it = copy.begin(); it != copy.end(); ++it)
  {
    bmData.m_featureTypes.push_back(c.GetIndexForType(*it));
    if (bmData.m_icon == kml::BookmarkIcon::None)
      bmData.m_icon = GetBookmarkIconByFeatureType(*it);
  }
}

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name)
{
  return kml::GetPreferredBookmarkStr(name, languages::GetCurrentNorm());
}

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name, feature::RegionData const & regionData)
{
  return kml::GetPreferredBookmarkStr(name, regionData, languages::GetCurrentNorm());
}

std::string GetLocalizedFeatureType(std::vector<uint32_t> const & types)
{
  return kml::GetLocalizedFeatureType(types);
}

std::string GetLocalizedBookmarkBaseType(BookmarkBaseType type)
{
  switch (type)
  {
  case BookmarkBaseType::None: return {};
  case BookmarkBaseType::Hotel: return platform::GetLocalizedString("hotels");
  case BookmarkBaseType::Animals: return platform::GetLocalizedString("animals");
  case BookmarkBaseType::Building: return platform::GetLocalizedString("buildings");
  case BookmarkBaseType::Entertainment: return platform::GetLocalizedString("entertainment");
  case BookmarkBaseType::Exchange: return platform::GetLocalizedString("money");
  case BookmarkBaseType::Food: return platform::GetLocalizedString("food_places");
  case BookmarkBaseType::Gas: return platform::GetLocalizedString("fuel_places");
  case BookmarkBaseType::Medicine: return platform::GetLocalizedString("medicine");
  case BookmarkBaseType::Mountain: return platform::GetLocalizedString("mountains");
  case BookmarkBaseType::Museum: return platform::GetLocalizedString("museums");
  case BookmarkBaseType::Park: return platform::GetLocalizedString("parks");
  case BookmarkBaseType::Parking: return platform::GetLocalizedString("parkings");
  case BookmarkBaseType::ReligiousPlace: return platform::GetLocalizedString("religious_places");
  case BookmarkBaseType::Shop: return platform::GetLocalizedString("shops");
  case BookmarkBaseType::Sights: return platform::GetLocalizedString("tourist_places");
  case BookmarkBaseType::Swim: return platform::GetLocalizedString("swim_places");
  case BookmarkBaseType::Water: return platform::GetLocalizedString("water");
  case BookmarkBaseType::Count: CHECK(false, ("Invalid bookmark base type")); return {};
  }
  UNREACHABLE();
}

std::string GetPreferredBookmarkName(kml::BookmarkData const & bmData)
{
  return kml::GetPreferredBookmarkName(bmData, languages::GetCurrentOrig());
}

bool FromCatalog(kml::FileData const & kmlData)
{
  return FromCatalog(kmlData.m_categoryData, kmlData.m_serverId);
}

bool FromCatalog(kml::CategoryData const & categoryData, std::string const & serverId)
{
  return !serverId.empty() && categoryData.m_accessRules != kml::AccessRules::Local;
}

bool IsMyCategory(std::string const & userId, kml::CategoryData const & categoryData)
{
  return userId == categoryData.m_authorId;
}

bool IsMyCategory(User const & user, kml::CategoryData const & categoryData)
{
  return IsMyCategory(user.GetUserId(), categoryData);
}

void ExpandBookmarksRectForPreview(m2::RectD & rect)
{
  if (!rect.IsValid())
    return;

  rect.Scale(df::kBoundingBoxScale);
}
