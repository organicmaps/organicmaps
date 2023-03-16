#pragma once

#include "generator/generate_info.hpp"

#include "indexer/data_header.hpp"

#include <string>

namespace feature
{
/// Final generation of data from input feature-file.
/// @param path - path to folder with countries;
/// @param name - name of generated country;
bool GenerateFinalFeatures(feature::GenerateInfo const & info, std::string const & name,
                           feature::DataHeader::MapType mapType);
}  // namespace feature
