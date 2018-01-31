#pragma once

#include <string>

namespace feature
{
/// Generates data for LocalityIndexBuilder from input feature-dat-files.
/// @param featuresDir - path to folder with pregenerated features data;
/// @param out - output file name;
bool GenerateLocalityData(std::string const & featuresDir, std::string const & out);
}  // namespace feature
