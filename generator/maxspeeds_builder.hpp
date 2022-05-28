#pragma once

#include "generator/routing_helpers.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace routing
{
class IndexGraph;
using OsmIdToMaxspeed = std::map<base::GeoObjectId, Maxspeed>;
} // namesoace routing

namespace routing_builder
{
/// \brief Parses csv file with |filePath| and stores the result in |osmIdToMaxspeed|.
/// \note There's a detailed description of the csv file in generator/maxspeed_collector.hpp.
bool ParseMaxspeeds(std::string const & filePath, routing::OsmIdToMaxspeed & osmIdToMaxspeed);

void BuildMaxspeedsSection(routing::IndexGraph * graph, std::string const & dataPath,
                           routing::FeatureIdToOsmId const & featureIdToOsmId,
                           std::string const & maxspeedsFilename);

/// \brief Builds maxspeeds section in mwm with |dataPath|. This section contains max speed limits
/// if they are available in file |maxspeedsFilename|.
/// \param maxspeedsFilename file name to csv file with maxspeed tag values.
/// \note To start building the section, the following steps should be done:
/// 1. Calls GenerateIntermediateData(). It stores data about maxspeed tags value of road features
//     to a csv file.
/// 2. Calls GenerateFeatures()
/// 3. Generates geometry.
void BuildMaxspeedsSection(routing::IndexGraph * graph, std::string const & dataPath,
                           std::string const & osmToFeaturePath, std::string const & maxspeedsFilename);
}  // namespace routing_builder
