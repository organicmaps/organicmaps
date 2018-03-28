#pragma once

#include <string>

namespace feature
{
/// Generates data for LocalityIndexBuilder from input feature-dat-files.
/// @param featuresDir - path to folder with pregenerated features data;
/// @param nodesFile - path to file with list of node ids we need to add to output;
/// @param out - output file name;
bool GenerateLocalityData(std::string const & featuresDir, std::string const & nodesFile,
                          std::string const & out);
}  // namespace feature
