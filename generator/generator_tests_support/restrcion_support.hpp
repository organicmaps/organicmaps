#pragma once

#include "std/string.hpp"

namespace generator
{
/// \brief Generates a binary file with by a string with mapping from osm ids to feature ids.
/// \param mappingContent a string with lines with mapping from osm id to feature id (one to one).
/// For example
/// 10, 1,
/// 20, 2
/// 30, 3,
/// 40, 4
/// \parma outputFilePath full path to an output file where the mapping is saved.
void GenerateOsmIdsToFeatureIdsMapping(string const & mappingContent, string const & outputFilePath);
}  // namespace generator
