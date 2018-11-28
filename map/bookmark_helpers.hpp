#pragma once

#include "map/bookmark.hpp"

#include "indexer/feature.hpp"

#include "coding/reader.hpp"

#include <memory>
#include <string>

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

bool SaveKmlFile(kml::FileData & kmlData, std::string const & file, KmlFileType fileType);
bool SaveKmlData(kml::FileData & kmlData, Writer & writer, KmlFileType fileType);

void ResetIds(kml::FileData & kmlData);

void SaveFeatureTypes(feature::TypesHolder const & types, kml::BookmarkData & bmData);

std::string GetPreferredBookmarkName(kml::BookmarkData const & bmData);

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name);
std::string GetPreferredBookmarkStr(kml::LocalizableString const & name, feature::RegionData const & regionData);
std::string GetLocalizedBookmarkType(std::vector<uint32_t> const & types);

bool FromCatalog(kml::FileData const & kmlData);
bool FromCatalog(kml::CategoryData const & categoryData, std::string const & serverId);
bool IsMyCategory(std::string const & userId, kml::CategoryData const & categoryData);
bool IsMyCategory(User const & user, kml::CategoryData const & categoryData);
