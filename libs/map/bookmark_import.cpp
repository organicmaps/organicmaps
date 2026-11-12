#include "map/bookmark_import.hpp"

#include "map/bookmark_manager.hpp"

#include "base/string_utils.hpp"
#include "geometry/mercator.hpp"

#include <bit>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <map>
#include <set>
#include <sstream>

namespace
{
char const * kSource = "om:import:source";
char const * kKey = "om:import:key";
char const * kSnapshot = "om:import:snapshot";
char const * kFingerprint = "om:import:fingerprint";
char const * kLatitude = "om:import:latitude";
char const * kLongitude = "om:import:longitude";
char const * kAppearance = "om:import:appearance";

bool IsFiniteCoordinate(double value)
{
  // std::isfinite can be optimized away in the project's fast-math builds.
  return (std::bit_cast<uint64_t>(value) & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL;
}

std::string const & Property(kml::Properties const & properties, char const * key)
{
  static std::string const empty;
  auto const it = properties.find(key);
  return it == properties.end() ? empty : it->second;
}

std::string Snapshot(std::string const & name, std::string const & address)
{
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::quoted(name) << std::quoted(address);
  return stream.str();
}

std::string Coordinate(double value)
{
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  // Align the source snapshot with normal OSM/KML coordinate precision.
  stream << std::fixed << std::setprecision(7) << value;
  return stream.str();
}

std::string Identity(std::string const & name, std::string const & address)
{
  auto normalize = [](std::string const & text)
  { return strings::MakeLowerCase(strings::JoinStrings(strings::TokenizeAndTrim<std::string>(text, " \t\r\n"), " ")); };
  std::ostringstream stream;
  stream << std::quoted(normalize(name)) << std::quoted(normalize(address));
  return stream.str();
}

std::string Appearance(kml::BookmarkData const & data)
{
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << static_cast<int>(data.m_icon) << ':' << static_cast<int>(data.m_color.m_predefinedColor) << ':'
         << data.m_color.m_rgba << ':' << static_cast<int>(data.m_viewportScale) << ':'
         << static_cast<int>(data.m_minZoom);
  stream << ':' << data.m_visible;
  for (auto const * names : {&data.m_name, &data.m_customName, &data.m_description})
  {
    stream << '|';
    for (auto const & [language, text] : *names)
      stream << static_cast<int>(language) << std::quoted(text);
  }
  for (auto type : data.m_featureTypes)
    stream << ':' << type;
  stream << '|';
  for (auto track : data.m_boundTracks)
    stream << ':' << track;
  for (auto const & [key, value] : data.m_properties)
    if (!key.starts_with("om:import:"))
      stream << std::quoted(key) << std::quoted(value);
  return stream.str();
}

bool Unmodified(Bookmark const & bookmark, bool strict)
{
  auto const & data = bookmark.GetData();
  auto const & properties = data.m_properties;
  double lat, lon;
  if (Property(properties, kSnapshot) != Snapshot(bookmark.GetPreferredName(), bookmark.GetDescription()) ||
      !strings::to_double(Property(properties, kLatitude), lat) ||
      !strings::to_double(Property(properties, kLongitude), lon) || !IsFiniteCoordinate(lat) ||
      !IsFiniteCoordinate(lon) || !mercator::ValidLat(lat) || !mercator::ValidLon(lon) ||
      mercator::DistanceOnEarth(bookmark.GetPivot(), mercator::FromLatLon(lat, lon)) > 1.0)
    return false;
  if (!strict)
    return true;
  auto const appearance = Property(properties, kAppearance);
  if (!appearance.empty())
    return appearance == Appearance(data);
  // Legacy imports have no appearance snapshot. Only migrate their untouched default style.
  for (auto const & [key, value] : properties)
    if (!key.starts_with("om:import:") && key != "CustomImage" && key != "CustomImageStyle")
      return false;
  return data.m_icon == kml::BookmarkIcon::None && data.m_color.m_predefinedColor == kml::PredefinedColor::Blue &&
         data.m_viewportScale == 0 && data.m_visible && data.m_boundTracks.empty() && data.m_featureTypes.empty() &&
         data.m_name.size() <= 1 && data.m_customName.size() <= 1 && data.m_description.size() <= 1 &&
         (data.m_minZoom == 1 || data.m_minZoom == 14 || data.m_minZoom == 16);
}
}  // namespace

std::vector<BookmarkImportEntry> GetImportedBookmarks(BookmarkManager const & manager, std::string const & source)
{
  std::vector<BookmarkImportEntry> entries;
  for (auto const category : manager.GetSortedBmGroupIdList())
    for (auto const id : manager.GetUserMarkIds(category))
    {
      auto const * bookmark = manager.GetBookmark(id);
      if (!bookmark)
        continue;
      auto const & properties = bookmark->GetData().m_properties;
      if (Property(properties, kSource) != source)
        continue;
      BookmarkImportEntry entry;
      entry.m_groupId = category;
      entry.m_canModify =
          Property(manager.GetCategoryData(category).m_properties, kSource) == source && Unmodified(*bookmark, true);
      entry.m_key = Property(properties, kKey);
      entry.m_sourceFingerprint = Property(properties, kFingerprint);
      std::istringstream snapshot(Property(properties, kSnapshot));
      snapshot.imbue(std::locale::classic());
      if (!(snapshot >> std::quoted(entry.m_name) >> std::quoted(entry.m_address)) || entry.m_key.empty() ||
          !strings::to_double(Property(properties, kLatitude), entry.m_lat) ||
          !strings::to_double(Property(properties, kLongitude), entry.m_lon) || !IsFiniteCoordinate(entry.m_lat) ||
          !IsFiniteCoordinate(entry.m_lon) || !mercator::ValidLat(entry.m_lat) || !mercator::ValidLon(entry.m_lon))
        continue;
      entries.push_back(std::move(entry));
    }
  return entries;
}

BookmarkImportResult ImportBookmarks(BookmarkManager & manager, std::string const & source,
                                     std::string const & categoryName, std::vector<BookmarkImportEntry> const & entries)
{
  BookmarkImportResult result;
  if (entries.empty() || source.empty() || categoryName.empty())
    return result;

  std::map<std::string, std::vector<kml::MarkId>> existing;
  std::map<std::string, std::vector<kml::MarkId>> fallback;
  std::set<std::string> incomingKeys;
  for (auto const & entry : entries)
    incomingKeys.insert(entry.m_key);
  for (auto const categoryId : manager.GetSortedBmGroupIdList())
  {
    if (Property(manager.GetCategoryData(categoryId).m_properties, kSource) == source &&
        result.m_categoryId == kml::kInvalidMarkGroupId)
      result.m_categoryId = categoryId;
    for (auto const id : manager.GetUserMarkIds(categoryId))
    {
      auto const * bookmark = manager.GetBookmark(id);
      if (!bookmark || Property(bookmark->GetData().m_properties, kSource) != source)
        continue;
      auto const & properties = bookmark->GetData().m_properties;
      auto const key = Property(properties, kKey);
      existing[key].push_back(id);
      // Recover changed provider keys only when the previous key is absent from this import.
      // Never merge two different current contacts just because they share a name and address.
      if (!incomingKeys.contains(key))
        fallback[Identity(bookmark->GetPreferredName(), bookmark->GetDescription())].push_back(id);
    }
  }

  auto session = manager.GetEditSession();
  std::set<std::string> processedKeys;
  std::set<kml::MarkId> claimed;
  auto retire = [&](BookmarkImportEntry const & entry, kml::MarkGroupId category)
  {
    if (Property(manager.GetCategoryData(category).m_properties, kSource) != source)
      return;
    for (auto const & key : entry.m_supersededKeys)
    {
      auto const it = existing.find(key);
      if (key == entry.m_key || incomingKeys.contains(key) || it == existing.end() || it->second.size() != 1)
        continue;
      auto const id = it->second.front();
      auto const * bookmark = manager.GetBookmark(id);
      if (bookmark && !claimed.contains(id) && bookmark->GetGroupId() == category && Unmodified(*bookmark, true))
      {
        claimed.insert(id);
        session.DeleteBookmark(id, false);
      }
    }
  };
  for (auto const & entry : entries)
  {
    if (entry.m_key.empty() || entry.m_name.empty() || !IsFiniteCoordinate(entry.m_lat) ||
        !IsFiniteCoordinate(entry.m_lon) || !mercator::ValidLat(entry.m_lat) || !mercator::ValidLon(entry.m_lon) ||
        !processedKeys.insert(entry.m_key).second)
    {
      ++result.m_skipped;
      continue;
    }

    kml::MarkId id = kml::kInvalidMarkId;
    auto const byKey = existing.find(entry.m_key);
    if (byKey != existing.end())
    {
      if (byKey->second.size() != 1)
      {
        ++result.m_skipped;
        continue;
      }
      id = byKey->second.front();
    }
    else if (auto const it = fallback.find(Identity(entry.m_name, entry.m_address)); it != fallback.end())
    {
      if (it->second.size() == 1 && !claimed.contains(it->second.front()))
        id = it->second.front();
      else
      {
        ++result.m_skipped;
        continue;
      }
    }
    auto const snapshot = Snapshot(entry.m_name, entry.m_address);
    auto const point = mercator::FromLatLon(entry.m_lat, entry.m_lon);
    kml::BookmarkData data;
    if (id != kml::kInvalidMarkId)
    {
      claimed.insert(id);
      auto const * bookmark = manager.GetBookmark(id);
      if (!bookmark ||
          (entry.m_requireSourceCategory &&
           Property(manager.GetCategoryData(bookmark->GetGroupId()).m_properties, kSource) != source) ||
          !Unmodified(*bookmark, entry.m_requireSourceCategory))
      {
        ++result.m_edited;
        continue;
      }
      data = bookmark->GetData();
      bool const needsPresentation = !entry.m_customImage.empty() && !data.m_properties.contains("CustomImage") &&
                                     !data.m_properties.contains("CustomImageOverride") &&
                                     data.m_icon == kml::BookmarkIcon::None &&
                                     data.m_color.m_predefinedColor == kml::PredefinedColor::Blue;
      bool const isPoiPresentation = Property(data.m_properties, "CustomImageStyle") == "poi";
      bool const needsPoiZoomUpdate =
          entry.m_poiStyle && isPoiPresentation &&
          (data.m_minZoom != 14 || Property(data.m_properties, "CustomImage") != entry.m_customImage);
      if (!needsPresentation && !needsPoiZoomUpdate &&
          (!entry.m_requireSourceCategory || !Property(data.m_properties, kAppearance).empty()) &&
          Property(data.m_properties, kSnapshot) == snapshot && Property(data.m_properties, kKey) == entry.m_key &&
          Property(data.m_properties, kFingerprint) == entry.m_sourceFingerprint &&
          mercator::DistanceOnEarth(bookmark->GetPivot(), point) <= 1.0)
      {
        ++result.m_unchanged;
        retire(entry, bookmark->GetGroupId());
        continue;
      }
    }
    else
    {
      if (result.m_categoryId == kml::kInvalidMarkGroupId)
      {
        std::string name = categoryName;
        for (size_t suffix = 2; manager.IsUsedCategoryName(name); ++suffix)
          name = categoryName + " " + std::to_string(suffix);
        result.m_categoryId = manager.CreateBookmarkCategory(name);
        session.SetCategoryCustomProperty(result.m_categoryId, kSource, source);
      }
      data.m_color.m_predefinedColor = kml::PredefinedColor::Blue;
      data.m_timestamp = kml::TimestampClock::now();
    }
    kml::SetDefaultStr(data.m_name, entry.m_name);
    kml::SetDefaultStr(data.m_customName, entry.m_name);
    kml::SetDefaultStr(data.m_description, entry.m_address);
    data.m_point = point;
    data.m_properties[kSource] = source;
    data.m_properties[kKey] = entry.m_key;
    data.m_properties[kSnapshot] = snapshot;
    if (entry.m_sourceFingerprint.empty())
      data.m_properties.erase(kFingerprint);
    else
      data.m_properties[kFingerprint] = entry.m_sourceFingerprint;
    data.m_properties[kLatitude] = Coordinate(entry.m_lat);
    data.m_properties[kLongitude] = Coordinate(entry.m_lon);
    if (!entry.m_customImage.empty() && !data.m_properties.contains("CustomImage") &&
        !data.m_properties.contains("CustomImageOverride") && data.m_icon == kml::BookmarkIcon::None &&
        data.m_color.m_predefinedColor == kml::PredefinedColor::Blue)
    {
      data.m_properties["CustomImage"] = entry.m_customImage;
      if (entry.m_poiStyle)
      {
        data.m_properties["CustomImageStyle"] = "poi";
        data.m_minZoom = 14;
      }
    }
    else if (entry.m_poiStyle && Property(data.m_properties, "CustomImageStyle") == "poi")
    {
      // Existing imports created before the marker became visible at scale 14.
      if (!entry.m_customImage.empty())
        data.m_properties["CustomImage"] = entry.m_customImage;
      data.m_minZoom = 14;
    }
    data.m_properties[kAppearance] = Appearance(data);
    if (id == kml::kInvalidMarkId)
    {
      // The interactive creation overload restores the last deleted bookmark and changes
      // category visibility/last-used settings. Import must not have those side effects.
      auto const * bookmark = session.CreateBookmark(std::move(data));
      session.AttachBookmark(bookmark->GetId(), result.m_categoryId);
      ++result.m_added;
      retire(entry, result.m_categoryId);
    }
    else
    {
      session.UpdateBookmark(id, data);
      ++result.m_updated;
      retire(entry, manager.GetBookmark(id)->GetGroupId());
    }
  }
  return result;
}
