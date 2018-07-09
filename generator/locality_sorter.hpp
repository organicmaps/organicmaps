#pragma once

#include <string>

namespace feature
{
// Generates data for GeoObjectsIndexBuilder from input feature-dat-files.
// @param featuresDir - path to folder with pregenerated features data;
// @param nodesFile - path to file with list of node ids we need to add to output;
// @param out - output file name;
bool GenerateGeoObjectsData(std::string const & featuresDir, std::string const & nodesFile,
                            std::string const & out);

// Generates data for RegionsIndexBuilder from input feature-dat-files.
// @param featuresDir - path to folder with pregenerated features data;
// @param out - output file name;
bool GenerateRegionsData(std::string const & featuresDir, std::string const & out);

// Generates borders section for server-side reverse geocoder from input feature-dat-files.
// @param featuresDir - path to folder with pregenerated features data;
// @param out - output file to add borders section;
bool GenerateBorders(std::string const & featuresDir, std::string const & out);
}  // namespace feature
