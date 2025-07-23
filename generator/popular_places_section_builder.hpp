#pragma once

#include "base/geo_object_id.hpp"

#include <string>
#include <unordered_map>

namespace generator
{
using PopularityIndex = uint8_t;
using PopularPlaces = std::unordered_map<base::GeoObjectId, PopularityIndex>;

void LoadPopularPlaces(std::string const & srcFilename, PopularPlaces & places);

void BuildPopularPlacesFromDescriptions(std::string const & mwmFile);
void BuildPopularPlacesFromWikiDump(std::string const & mwmFile, std::string const & wikipediaDir,
                                    std::string const & idToWikidataPath);

PopularPlaces const & GetOrLoadPopularPlaces(std::string const & filename);
}  // namespace generator
