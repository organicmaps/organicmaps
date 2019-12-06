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
  {"amenity-veterinary", kml::BookmarkIcon::Animals},
  {"leisure-dog_park", kml::BookmarkIcon::Animals},
  {"tourism-zoo", kml::BookmarkIcon::Animals},

  {"amenity-bar", kml::BookmarkIcon::Bar},
  {"amenity-biergarten", kml::BookmarkIcon::Bar},
  {"amenity-pub", kml::BookmarkIcon::Bar},

  {"amenity-place_of_worship-buddhist", kml::BookmarkIcon::Buddhism},

  {"amenity-college", kml::BookmarkIcon::Building},
  {"amenity-courthouse", kml::BookmarkIcon::Building},
  {"amenity-embassy", kml::BookmarkIcon::Building},
  {"amenity-kindergarten", kml::BookmarkIcon::Building},
  {"amenity-library", kml::BookmarkIcon::Building},
  {"amenity-police", kml::BookmarkIcon::Building},
  {"amenity-prison", kml::BookmarkIcon::Building},
  {"amenity-school", kml::BookmarkIcon::Building},
  {"building-train_station", kml::BookmarkIcon::Building},
  {"building-university", kml::BookmarkIcon::Building},
  {"office", kml::BookmarkIcon::Building},
  {"office-lawyer", kml::BookmarkIcon::Building},

  {"amenity-grave_yard-christian", kml::BookmarkIcon::Christianity},
  {"amenity-place_of_worship-christian", kml::BookmarkIcon::Christianity},
  {"landuse-cemetery-christian", kml::BookmarkIcon::Christianity},

  {"amenity-casino", kml::BookmarkIcon::Entertainment},
  {"amenity-cinema", kml::BookmarkIcon::Entertainment},
  {"amenity-nightclub", kml::BookmarkIcon::Entertainment},
  {"amenity-theatre", kml::BookmarkIcon::Entertainment},
  {"leisure-playground", kml::BookmarkIcon::Entertainment},
  {"leisure-water_park", kml::BookmarkIcon::Entertainment},
  {"shop-bookmaker", kml::BookmarkIcon::Entertainment},
  {"tourism-theme_park", kml::BookmarkIcon::Entertainment},

  {"amenity-atm", kml::BookmarkIcon::Exchange},
  {"amenity-bank", kml::BookmarkIcon::Exchange},
  {"amenity-bureau_de_change", kml::BookmarkIcon::Exchange},

  {"amenity-bbq", kml::BookmarkIcon::Food},
  {"amenity-cafe", kml::BookmarkIcon::Food},
  {"amenity-fast_food", kml::BookmarkIcon::Food},
  {"amenity-food_court", kml::BookmarkIcon::Food},
  {"amenity-restaurant", kml::BookmarkIcon::Food},
  {"leisure-picnic_table", kml::BookmarkIcon::Food},
  {"tourism-picnic_site", kml::BookmarkIcon::Food},

  {"amenity-charging_station", kml::BookmarkIcon::Gas},
  {"amenity-fuel", kml::BookmarkIcon::Gas},

  {"sponsored-booking", kml::BookmarkIcon::Hotel},
  {"tourism-alpine_hut", kml::BookmarkIcon::Hotel},
  {"tourism-camp_site", kml::BookmarkIcon::Hotel},
  {"tourism-chalet", kml::BookmarkIcon::Hotel},
  {"tourism-guest_house", kml::BookmarkIcon::Hotel},
  {"tourism-hostel", kml::BookmarkIcon::Hotel},
  {"tourism-hotel", kml::BookmarkIcon::Hotel},
  {"tourism-motel", kml::BookmarkIcon::Hotel},
  {"tourism-resort", kml::BookmarkIcon::Hotel},
  {"tourism-wilderness_hut", kml::BookmarkIcon::Hotel},
  {"tourism-apartment", kml::BookmarkIcon::Hotel},

  {"amenity-place_of_worship-muslim", kml::BookmarkIcon::Islam},

  {"amenity-place_of_worship-jewish", kml::BookmarkIcon::Judaism},

  {"amenity-childcare", kml::BookmarkIcon::Medicine},
  {"amenity-clinic", kml::BookmarkIcon::Medicine},
  {"amenity-dentist", kml::BookmarkIcon::Medicine},
  {"amenity-doctors", kml::BookmarkIcon::Medicine},
  {"amenity-hospital", kml::BookmarkIcon::Medicine},
  {"amenity-pharmacy", kml::BookmarkIcon::Medicine},
  {"emergency-defibrillator", kml::BookmarkIcon::Medicine},

  {"natural-bare_rock", kml::BookmarkIcon::Mountain},
  {"natural-cave_entrance", kml::BookmarkIcon::Mountain},
  {"natural-peak", kml::BookmarkIcon::Mountain},
  {"natural-rock", kml::BookmarkIcon::Mountain},
  {"natural-volcano", kml::BookmarkIcon::Mountain},

  {"amenity-arts_centre", kml::BookmarkIcon::Museum},
  {"tourism-gallery", kml::BookmarkIcon::Museum},
  {"tourism-museum", kml::BookmarkIcon::Museum},

  {"boundary-national_park", kml::BookmarkIcon::Park},
  {"landuse-forest", kml::BookmarkIcon::Park},
  {"leisure-garden", kml::BookmarkIcon::Park},
  {"leisure-nature_reserve", kml::BookmarkIcon::Park},
  {"leisure-park", kml::BookmarkIcon::Park},

  {"amenity-bicycle_parking", kml::BookmarkIcon::Parking},
  {"amenity-motorcycle_parking", kml::BookmarkIcon::Parking},
  {"amenity-parking", kml::BookmarkIcon::Parking},
  {"highway-services", kml::BookmarkIcon::Parking},
  {"tourism-caravan_site", kml::BookmarkIcon::Parking},
  {"vending-parking_tickets", kml::BookmarkIcon::Parking},

  {"amenity-ice_cream", kml::BookmarkIcon::Shop},
  {"amenity-marketplace", kml::BookmarkIcon::Shop},
  {"amenity-vending_machine", kml::BookmarkIcon::Shop},
  {"shop", kml::BookmarkIcon::Shop},

  {"amenity-place_of_worship", kml::BookmarkIcon::Sights},
  {"historic-archaeological_site", kml::BookmarkIcon::Sights},
  {"historic-boundary_stone", kml::BookmarkIcon::Sights},
  {"historic-castle", kml::BookmarkIcon::Sights},
  {"historic-fort", kml::BookmarkIcon::Sights},
  {"historic-memorial", kml::BookmarkIcon::Sights},
  {"historic-monument", kml::BookmarkIcon::Sights},
  {"historic-ruins", kml::BookmarkIcon::Sights},
  {"historic-ship", kml::BookmarkIcon::Sights},
  {"historic-tomb", kml::BookmarkIcon::Sights},
  {"historic-wayside_cross", kml::BookmarkIcon::Sights},
  {"historic-wayside_shrine", kml::BookmarkIcon::Sights},
  {"tourism-artwork", kml::BookmarkIcon::Sights},
  {"tourism-attraction", kml::BookmarkIcon::Sights},
  {"waterway-waterfall", kml::BookmarkIcon::Sights},

  {"leisure-fitness_centre", kml::BookmarkIcon::Sport},
  {"leisure-skiing", kml::BookmarkIcon::Sport},
  {"leisure-sports_centre-climbing", kml::BookmarkIcon::Sport},
  {"leisure-sports_centre-shooting", kml::BookmarkIcon::Sport},
  {"leisure-sports_centre-yoga", kml::BookmarkIcon::Sport},
  {"leisure-stadium", kml::BookmarkIcon::Sport},
  {"olympics-bike_sport", kml::BookmarkIcon::Sport},
  {"olympics-stadium", kml::BookmarkIcon::Sport},
  {"olympics-stadium_main", kml::BookmarkIcon::Sport},
  {"sport", kml::BookmarkIcon::Sport},

  {"leisure-sports_centre-swimming", kml::BookmarkIcon::Swim},
  {"leisure-swimming_pool", kml::BookmarkIcon::Swim},
  {"natural-beach", kml::BookmarkIcon::Swim},
  {"olympics-water_sport", kml::BookmarkIcon::Swim},
  {"sport-diving", kml::BookmarkIcon::Swim},
  {"sport-scuba_diving", kml::BookmarkIcon::Swim},
  {"sport-swimming", kml::BookmarkIcon::Swim},

  {"aeroway-aerodrome", kml::BookmarkIcon::Transport},
  {"aeroway-aerodrome-international", kml::BookmarkIcon::Transport},
  {"amenity-bus_station", kml::BookmarkIcon::Transport},
  {"amenity-car_sharing", kml::BookmarkIcon::Transport},
  {"amenity-ferry_terminal", kml::BookmarkIcon::Transport},
  {"amenity-taxi", kml::BookmarkIcon::Transport},
  {"building-train_station", kml::BookmarkIcon::Transport},
  {"highway-bus_stop", kml::BookmarkIcon::Transport},
  {"highway-platform", kml::BookmarkIcon::Transport},
  {"public_transport-platform", kml::BookmarkIcon::Transport},
  {"railway-station", kml::BookmarkIcon::Transport},
  {"railway-station-light_rail", kml::BookmarkIcon::Transport},
  {"railway-station-monorail", kml::BookmarkIcon::Transport},
  {"railway-station-subway", kml::BookmarkIcon::Transport},
  {"railway-tram_stop", kml::BookmarkIcon::Transport},

  {"tourism-viewpoint", kml::BookmarkIcon::Viewpoint},

  {"amenity-drinking_water", kml::BookmarkIcon::Water},
  {"amenity-fountain", kml::BookmarkIcon::Water},
  {"amenity-water_point", kml::BookmarkIcon::Water},
  {"man_made-water_tap", kml::BookmarkIcon::Water},
  {"natural-spring", kml::BookmarkIcon::Water},

  {"shop-funeral_directors", kml::BookmarkIcon::None}
};

std::map<std::string, BookmarkBaseType> const kFeatureTypeToBookmarkType = {
  {"amenity-veterinary", BookmarkBaseType::Animals},
  {"leisure-dog_park", BookmarkBaseType::Animals},
  {"tourism-zoo", BookmarkBaseType::Animals},

  {"amenity-college", BookmarkBaseType::Building},
  {"amenity-courthouse", BookmarkBaseType::Building},
  {"amenity-embassy", BookmarkBaseType::Building},
  {"amenity-kindergarten", BookmarkBaseType::Building},
  {"amenity-library", BookmarkBaseType::Building},
  {"amenity-police", BookmarkBaseType::Building},
  {"amenity-prison", BookmarkBaseType::Building},
  {"amenity-school", BookmarkBaseType::Building},
  {"building-train_station", BookmarkBaseType::Building},
  {"building-university", BookmarkBaseType::Building},
  {"office", BookmarkBaseType::Building},
  {"office-lawyer", BookmarkBaseType::Building},

  {"amenity-casino", BookmarkBaseType::Entertainment},
  {"amenity-cinema", BookmarkBaseType::Entertainment},
  {"amenity-nightclub", BookmarkBaseType::Entertainment},
  {"amenity-theatre", BookmarkBaseType::Entertainment},
  {"leisure-water_park", BookmarkBaseType::Entertainment},
  {"leisure-playground", BookmarkBaseType::Entertainment},
  {"shop-bookmaker", BookmarkBaseType::Entertainment},

  // Sport
  {"leisure-fitness_centre", BookmarkBaseType::Entertainment},
  {"leisure-sports_centre-climbing", BookmarkBaseType::Entertainment},
  {"leisure-sports_centre-shooting", BookmarkBaseType::Entertainment},
  {"leisure-sports_centre-yoga", BookmarkBaseType::Entertainment},
  {"leisure-stadium", BookmarkBaseType::Entertainment},
  {"olympics-bike_sport", BookmarkBaseType::Entertainment},
  {"olympics-stadium", BookmarkBaseType::Entertainment},
  {"olympics-stadium_main", BookmarkBaseType::Entertainment},
  {"sport", BookmarkBaseType::Entertainment},
  {"tourism-theme_park", BookmarkBaseType::Entertainment},

  {"amenity-atm", BookmarkBaseType::Exchange},
  {"amenity-bank", BookmarkBaseType::Exchange},
  {"amenity-bureau_de_change", BookmarkBaseType::Exchange},

  {"amenity-bbq", BookmarkBaseType::Food},
  {"amenity-cafe", BookmarkBaseType::Food},
  {"amenity-fast_food", BookmarkBaseType::Food},
  {"amenity-food_court", BookmarkBaseType::Food},
  {"amenity-restaurant", BookmarkBaseType::Food},
  {"leisure-picnic_table", BookmarkBaseType::Food},
  {"tourism-picnic_site", BookmarkBaseType::Food},
  // Bars
  {"amenity-bar", BookmarkBaseType::Food},
  {"amenity-biergarten", BookmarkBaseType::Food},
  {"amenity-pub", BookmarkBaseType::Food},

  {"amenity-charging_station", BookmarkBaseType::Gas},
  {"amenity-fuel", BookmarkBaseType::Gas},

  {"sponsored-booking", BookmarkBaseType::Hotel},
  {"tourism-alpine_hut", BookmarkBaseType::Hotel},
  {"tourism-apartment", BookmarkBaseType::Hotel},
  {"tourism-camp_site", BookmarkBaseType::Hotel},
  {"tourism-chalet", BookmarkBaseType::Hotel},
  {"tourism-guest_house", BookmarkBaseType::Hotel},
  {"tourism-hostel", BookmarkBaseType::Hotel},
  {"tourism-hotel", BookmarkBaseType::Hotel},
  {"tourism-motel", BookmarkBaseType::Hotel},
  {"tourism-resort", BookmarkBaseType::Hotel},
  {"tourism-wilderness_hut", BookmarkBaseType::Hotel},

  {"amenity-childcare", BookmarkBaseType::Medicine},
  {"amenity-clinic", BookmarkBaseType::Medicine},
  {"amenity-dentist", BookmarkBaseType::Medicine},
  {"amenity-doctors", BookmarkBaseType::Medicine},
  {"amenity-hospital", BookmarkBaseType::Medicine},
  {"amenity-pharmacy", BookmarkBaseType::Medicine},
  {"emergency-defibrillator", BookmarkBaseType::Medicine},

  {"natural-bare_rock", BookmarkBaseType::Mountain},
  {"natural-cave_entrance", BookmarkBaseType::Mountain},
  {"natural-peak", BookmarkBaseType::Mountain},
  {"natural-rock", BookmarkBaseType::Mountain},
  {"natural-volcano", BookmarkBaseType::Mountain},

  {"amenity-arts_centre", BookmarkBaseType::Museum},
  {"tourism-gallery", BookmarkBaseType::Museum},
  {"tourism-museum", BookmarkBaseType::Museum},

  {"boundary-national_park", BookmarkBaseType::Park},
  {"landuse-forest", BookmarkBaseType::Park},
  {"leisure-garden", BookmarkBaseType::Park},
  {"leisure-nature_reserve", BookmarkBaseType::Park},
  {"leisure-park", BookmarkBaseType::Park},

  {"amenity-bicycle_parking", BookmarkBaseType::Parking},
  {"amenity-motorcycle_parking", BookmarkBaseType::Parking},
  {"amenity-parking", BookmarkBaseType::Parking},
  {"highway-services", BookmarkBaseType::Parking},
  {"tourism-caravan_site", BookmarkBaseType::Parking},
  {"vending-parking_tickets", BookmarkBaseType::Parking},

  // Christianity
  {"amenity-grave_yard-christian", BookmarkBaseType::ReligiousPlace},
  {"amenity-place_of_worship-christian", BookmarkBaseType::ReligiousPlace},
  {"landuse-cemetery-christian", BookmarkBaseType::ReligiousPlace},
  // Judaism
  {"amenity-place_of_worship-jewish", BookmarkBaseType::ReligiousPlace},
  // Buddhism
  {"amenity-place_of_worship-buddhist", BookmarkBaseType::ReligiousPlace},
  // Islam
  {"amenity-place_of_worship-muslim", BookmarkBaseType::ReligiousPlace},
  // Sights
  {"amenity-place_of_worship", BookmarkBaseType::ReligiousPlace},

  {"amenity-ice_cream", BookmarkBaseType::Shop},
  {"amenity-marketplace", BookmarkBaseType::Shop},
  {"amenity-vending_machine", BookmarkBaseType::Shop},
  {"shop", BookmarkBaseType::Shop},

  {"historic-archaeological_site", BookmarkBaseType::Sights},
  {"historic-boundary_stone", BookmarkBaseType::Sights},
  {"historic-castle", BookmarkBaseType::Sights},
  {"historic-fort", BookmarkBaseType::Sights},
  {"historic-memorial", BookmarkBaseType::Sights},
  {"historic-monument", BookmarkBaseType::Sights},
  {"historic-ruins", BookmarkBaseType::Sights},
  {"historic-ship", BookmarkBaseType::Sights},
  {"historic-tomb", BookmarkBaseType::Sights},
  {"historic-wayside_cross", BookmarkBaseType::Sights},
  {"historic-wayside_shrine", BookmarkBaseType::Sights},
  {"tourism-artwork", BookmarkBaseType::Sights},
  {"tourism-attraction", BookmarkBaseType::Sights},
  {"waterway-waterfall", BookmarkBaseType::Sights},
  // Viewpoint
  {"tourism-viewpoint", BookmarkBaseType::Sights},

  {"leisure-sports_centre-swimming", BookmarkBaseType::Swim},
  {"leisure-swimming_pool", BookmarkBaseType::Swim},
  {"natural-beach", BookmarkBaseType::Swim},
  {"olympics-water_sport", BookmarkBaseType::Swim},
  {"sport-diving", BookmarkBaseType::Swim},
  {"sport-scuba_diving", BookmarkBaseType::Swim},
  {"sport-swimming", BookmarkBaseType::Swim},

  {"amenity-drinking_water", BookmarkBaseType::Water},
  {"amenity-fountain", BookmarkBaseType::Water},
  {"amenity-water_point", BookmarkBaseType::Water},
  {"man_made-water_tap", BookmarkBaseType::Water},
  {"natural-spring", BookmarkBaseType::Water},

  {"shop-funeral_directors", BookmarkBaseType::None}
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
