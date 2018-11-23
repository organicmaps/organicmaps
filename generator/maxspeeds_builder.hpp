#pragma once

#include "routing_common/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace routing
{
using OsmIdToMaxspeed = std::map<base::GeoObjectId, Maxspeed>;

/// \brief Parses csv file with path |maxspeedsFilename| and stores the result in |osmIdToMaxspeed|.
/// \note There's a detailed description of the csv file in generator/maxspeed_collector.hpp.
bool ParseMaxspeeds(std::string const & maxspeedsFilename, OsmIdToMaxspeed & osmIdToMaxspeed);

/// \brief Writes |speeds| to maxspeeds section to mwm with |dataPath|.
void SerializeMaxspeeds(std::string const & dataPath, std::vector<FeatureMaxspeed> && speeds);

void BuildMaxspeedsSection(std::string const & dataPath,
                           std::map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                           std::string const & maxspeedsFilename);

/// \brief Builds maxspeeds section in mwm with |dataPath|. This section contains max speed limits
/// if they are available in file |maxspeedsFilename|.
/// \param maxspeedsFilename file name to csv file with maxspeed tag values.
/// \note To start building the section, the following steps should be done:
/// 1. Calls GenerateIntermediateData(). It stores data about maxspeed tags value of road features
//     to a csv file.
/// 2. Calls GenerateFeatures()
/// 3. Generates geometry.
void BuildMaxspeedsSection(std::string const & dataPath, std::string const & osmToFeaturePath,
                           std::string const & maxspeedsFilename);
}  // namespace routing
