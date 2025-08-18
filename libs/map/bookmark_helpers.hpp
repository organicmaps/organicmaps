#pragma once

#include "map/bookmark.hpp"

#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include <memory>
#include <string>

struct BookmarkInfo
{
  BookmarkInfo() = default;

  BookmarkInfo(kml::MarkId id, kml::BookmarkData const & data) : m_bookmarkId(id), m_bookmarkData(data) {}

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

std::string_view constexpr kKmzExtension = ".kmz";
std::string_view constexpr kKmlExtension = ".kml";
std::string_view constexpr kKmbExtension = ".kmb";
std::string_view constexpr kGpxExtension = ".gpx";

std::string_view constexpr kTrashDirectoryName = ".Trash";

extern std::string const kDefaultBookmarksFileName;

enum class KmlFileType
{
  Text,
  Binary,
  Gpx
};

inline std::string DebugPrint(KmlFileType fileType)
{
  switch (fileType)
  {
  case KmlFileType::Text: return "Text";
  case KmlFileType::Binary: return "Binary";
  case KmlFileType::Gpx: return "GPX";
  }
  UNREACHABLE();
}

/// @name File name/path helpers.
/// @{
std::string GetBookmarksDirectory();
std::string GetTrashDirectory();
std::string RemoveInvalidSymbols(std::string const & name);
std::string GenerateUniqueFileName(std::string const & path, std::string name, std::string_view ext = kKmlExtension);
std::string GenerateValidAndUniqueFilePathForKML(std::string const & fileName);
std::string GenerateValidAndUniqueFilePathForGPX(std::string const & fileName);
std::string GenerateValidAndUniqueTrashedFilePath(std::string const & fileName);
/// @}

/// @name SerDes helpers.
/// @{
std::unique_ptr<kml::FileData> LoadKmlFile(std::string const & file, KmlFileType fileType);
std::unique_ptr<kml::FileData> LoadKmlData(Reader const & reader, KmlFileType fileType);

std::vector<std::string> GetKMLOrGPXFilesPathsToLoad(std::string const & filePath);
std::vector<std::string> GetFilePathsToLoadFromKml(std::string const & filePath);
std::vector<std::string> GetFilePathsToLoadFromGpx(std::string const & filePath);
std::vector<std::string> GetFilePathsToLoadFromKmb(std::string const & filePath);
std::vector<std::string> GetFilePathsToLoadFromKmz(std::string const & filePath);
std::string GetLowercaseFileExt(std::string const & filePath);

bool SaveKmlFileSafe(kml::FileData & kmlData, std::string const & file, KmlFileType fileType);
bool SaveKmlData(kml::FileData & kmlData, Writer & writer, KmlFileType fileType);
bool SaveKmlFileByExt(kml::FileData & kmlData, std::string const & file);
/// @}

void ResetIds(kml::FileData & kmlData);

namespace feature
{
class TypesHolder;
}
void SaveFeatureTypes(feature::TypesHolder const & types, kml::BookmarkData & bmData);

std::string GetPreferredBookmarkName(kml::BookmarkData const & bmData);

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name);
std::string GetPreferredBookmarkStr(kml::LocalizableString const & name, feature::RegionData const & regionData);
std::string GetLocalizedFeatureType(std::vector<uint32_t> const & types);
std::string GetLocalizedBookmarkBaseType(BookmarkBaseType type);

kml::BookmarkIcon GetBookmarkIconByFeatureType(uint32_t type);
BookmarkBaseType GetBookmarkBaseType(std::vector<uint32_t> const & featureTypes);

void ExpandRectForPreview(m2::RectD & rect);
