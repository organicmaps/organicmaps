#pragma once
#include "generator/generate_info.hpp"

#include <string>

namespace feature
{
/// Final generation of data from input feature-dat-file.
/// @param path - path to folder with countries;
/// @param name - name of generated country;
bool GenerateFinalFeatures(feature::GenerateInfo const & info, std::string const & name,
                           int mapType);
}  // namespace feature
