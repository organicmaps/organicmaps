#pragma once

#include <string>

namespace generator
{
/// \brief Generates a binary file by |mappingContent| with mapping from osm ids to feature ids.
/// \param mappingContent a string with lines with mapping from osm id to feature id (one to one).
/// For example
/// 10, 1,
/// 20, 2
/// 30, 3,
/// 40, 4
/// \param outputFilePath full path to an output file where the mapping is saved.
void ReEncodeOsmIdsToFeatureIdsMapping(std::string const & mappingContent,
                                       std::string const & outputFilePath);
}  // namespace generator
