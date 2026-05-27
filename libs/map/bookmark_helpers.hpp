#pragma once

#include "kml/types.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/rect2d.hpp"

#include <memory>
#include <string>

// Lightweight non-owning view of a bookmark, delivered to BookmarkManager callbacks.
// The pointed-to BookmarkData must outlive the BookmarkInfo: callbacks are invoked
// synchronously while the bookmark is alive in the BookmarkManager.
struct BookmarkInfo
{
  BookmarkInfo(kml::MarkId id, kml::BookmarkData const * data) : m_bookmarkId(id), m_bookmarkData(data) {}

  kml::MarkId m_bookmarkId;
  kml::BookmarkData const * m_bookmarkData;
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
std::string_view constexpr kGeoJsonExtension = ".geojson";
std::string_view constexpr kJsonExtension = ".json";

std::string_view constexpr kTrashDirectoryName = ".Trash";

extern std::string const kDefaultBookmarksFileName;

enum class FileType
{
  Kml,
  Kmz,
  Kmb,
  Gpx,
  GeoJson,
  Json
};

inline std::string_view GetFileTypeExtension(FileType fileType)
{
  switch (fileType)
  {
  case FileType::Kml: return kKmlExtension;
  case FileType::Kmz: return kKmzExtension;
  case FileType::Kmb: return kKmbExtension;
  case FileType::Gpx: return kGpxExtension;
  case FileType::GeoJson: return kGeoJsonExtension;
  case FileType::Json: return kJsonExtension;
  }
  UNREACHABLE();
}

inline std::string DebugPrint(FileType const fileType)
{
  return "FileType [" + std::string{GetFileTypeExtension(fileType)} + "]";
}

/// @name File name/path helpers.
/// @{

// File-name limit per component, in the units the local filesystem uses for
// NAME_MAX: UTF-16 code units on Apple (APFS/HFS+) and UTF-8 bytes elsewhere
// (ext4/F2FS on Linux/Android, NTFS on Windows). 200 leaves headroom inside
// the kernel's 255-unit budget for ".geojson", the ".tmp<thread_id>" suffix
// from WriteToTempAndRenameToFile, and GenerateUniqueFileName's collision
// counter.
inline constexpr size_t kMaxFileNameLength = 200;

std::string GetBookmarksDirectory();
std::string GetTrashDirectory();
std::string RemoveInvalidSymbols(std::string const & name);
// Truncates `name` (UTF-8) so its on-disk length stays within kMaxFileNameLength.
// On Apple platforms a supplementary-plane codepoint counts as 2 UTF-16 units
// (it becomes a surrogate pair); elsewhere only UTF-8 bytes matter. Always
// cuts at a codepoint boundary -- never produces invalid UTF-8. Short names
// are returned unchanged.
std::string TruncateToValidFileName(std::string name);
std::string GenerateUniqueFileName(std::string const & path, std::string name, std::string_view ext = kKmlExtension);
std::string GenerateValidAndUniqueTrashedFilePath(std::string const & fileName);
std::string GenerateValidAndUniqueFilePath(std::string const & fileName, FileType const fileType);
/// @}

/// @name SerDes helpers.
/// @{
std::unique_ptr<kml::FileData> LoadKmlFile(std::string const & file, FileType fileType);
std::unique_ptr<kml::FileData> LoadKmlData(Reader const & reader, FileType fileType);

std::vector<std::string> GetKMLOrGPXFilesPathsToLoad(std::string const & filePath);
std::string GetLowercaseFileExt(std::string const & filePath);
std::optional<FileType> GetFileType(std::string const & filePath);

bool SaveKmlFileSafe(kml::FileData const & kmlData, std::string const & file, FileType fileType);
bool SaveKmlData(kml::FileData const & kmlData, Writer & writer, FileType fileType);
bool SaveKmlFileByExt(kml::FileData const & kmlData, std::string const & file);
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
std::string GetLocalizedBookmarkBaseType(BookmarkBaseType type);

struct BookmarkMatchInfo
{
  kml::BookmarkIcon m_icon;
  BookmarkBaseType m_type;

  bool operator==(BookmarkMatchInfo const &) const = default;
};
BookmarkMatchInfo GetBookmarkMatchInfo(uint32_t type);
inline BookmarkMatchInfo GetBookmarkMatchInfo(std::vector<uint32_t> const & types)
{
  for (auto t : types)
    if (auto info = GetBookmarkMatchInfo(t); info.m_icon != kml::BookmarkIcon::None)
      return info;
  return {kml::BookmarkIcon::None, BookmarkBaseType::None};
}

void ExpandRectForPreview(m2::RectD & rect);
