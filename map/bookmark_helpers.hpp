#pragma once

#include "map/bookmark.hpp"

#include "indexer/feature.hpp"

#include "coding/reader.hpp"

#include <memory>
#include <string>

std::unique_ptr<kml::FileData> LoadKmlFile(std::string const & file, bool useBinary);
std::unique_ptr<kml::FileData> LoadKmlData(Reader const & reader, bool useBinary);

bool SaveKmlFile(kml::FileData & kmlData, std::string const & file, bool useBinary);
bool SaveKmlData(kml::FileData & kmlData, Writer & writer, bool useBinary);

void ResetIds(kml::FileData & kmlData);

void SaveFeatureTypes(feature::TypesHolder const & types, kml::BookmarkData & bmData);

std::string GetPreferredBookmarkName(kml::BookmarkData const & bmData);

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name);
std::string GetPreferredBookmarkStr(kml::LocalizableString const & name, feature::RegionData const & regionData);
std::string GetLocalizedBookmarkType(std::vector<uint32_t> const & types);
