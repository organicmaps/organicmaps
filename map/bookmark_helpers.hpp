#pragma once

#include "map/bookmark.hpp"

#include "indexer/feature.hpp"

#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include <memory>
#include <string>

struct BookmarkInfo
{
  BookmarkInfo() = default;

  BookmarkInfo(kml::MarkId id, kml::BookmarkData const & data)
    : m_bookmarkId(id)
    , m_bookmarkData(data)
  {}

  BookmarkInfo(kml::MarkId id, kml::BookmarkData const & data, search::ReverseGeocoder::RegionAddress const & address)
    : m_bookmarkId(id)
    , m_bookmarkData(data)
    , m_address(address)
  {}

  kml::MarkId m_bookmarkId;
  kml::BookmarkData m_bookmarkData;
  search::ReverseGeocoder::RegionAddress m_address;
};

struct BookmarkGroupInfo
{
  BookmarkGroupInfo() = default;
  BookmarkGroupInfo(kml::MarkGroupId id, kml::MarkIdCollection && marks)
    : m_groupId(id)
    , m_bookmarkIds(std::move(marks))
  {}

  kml::MarkGroupId m_groupId;
  kml::MarkIdCollection m_bookmarkIds;
};

// Do not change the order.
enum class BookmarkBaseType : uint16_t
{
  None = 0,
  Hotel,
  Animals,
  Building,
  Entertainment,
  Exchange,
  Food,
  Gas,
  Medicine,
  Mountain,
  Museum,
  Park,
  Parking,
  ReligiousPlace,
  Shop,
  Sights,
  Swim,
  Water,
  Count
};

extern std::string const kKmzExtension;
extern std::string const kKmlExtension;
extern std::string const kKmbExtension;

class User;

enum class KmlFileType
{
  Text,
  Binary
};

inline std::string DebugPrint(KmlFileType fileType)
{
  switch (fileType)
  {
  case KmlFileType::Text: return "Text";
  case KmlFileType::Binary: return "Binary";
  }
  UNREACHABLE();
}

std::unique_ptr<kml::FileData> LoadKmlFile(std::string const & file, KmlFileType fileType);
std::unique_ptr<kml::FileData> LoadKmzFile(std::string const & file, std::string & kmlHash);
std::unique_ptr<kml::FileData> LoadKmlData(Reader const & reader, KmlFileType fileType);

bool SaveKmlFileSafe(kml::FileData & kmlData, std::string const & file, KmlFileType fileType);
bool SaveKmlData(kml::FileData & kmlData, Writer & writer, KmlFileType fileType);

void ResetIds(kml::FileData & kmlData);

void SaveFeatureTypes(feature::TypesHolder const & types, kml::BookmarkData & bmData);

std::string GetPreferredBookmarkName(kml::BookmarkData const & bmData);

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name);
std::string GetPreferredBookmarkStr(kml::LocalizableString const & name, feature::RegionData const & regionData);
std::string GetLocalizedFeatureType(std::vector<uint32_t> const & types);
std::string GetLocalizedBookmarkBaseType(BookmarkBaseType type);

kml::BookmarkIcon GetBookmarkIconByFeatureType(uint32_t type);
BookmarkBaseType GetBookmarkBaseType(std::vector<uint32_t> const & featureTypes);

bool FromCatalog(kml::FileData const & kmlData);
bool FromCatalog(kml::CategoryData const & categoryData, std::string const & serverId);
bool IsMyCategory(std::string const & userId, kml::CategoryData const & categoryData);
bool IsMyCategory(User const & user, kml::CategoryData const & categoryData);

void ExpandRectForPreview(m2::RectD & rect);
