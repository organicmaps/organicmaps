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

void SaveFeatureInfo(StringUtf8Multilang const & name, feature::TypesHolder const & types,
                     kml::BookmarkData & bmData);
