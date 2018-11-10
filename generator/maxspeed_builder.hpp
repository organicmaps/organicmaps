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

/// \brief Parses csv file with path |maxspeedFilename| and keep the result in |osmIdToMaxspeed|.
/// \note There's a detailed description of the csv file in generator/maxspeed_collector.hpp.
bool ParseMaxspeeds(std::string const & maxspeedFilename, OsmIdToMaxspeed & osmIdToMaxspeed);

/// \brief Write |speeds| to maxspeed section to mwm with |dataPath|.
void SerializeMaxspeed(std::string const & dataPath, std::vector<FeatureMaxspeed> && speeds);

void BuildMaxspeed(std::string const & dataPath,
                   std::map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                   std::string const & maxspeedFilename);

/// \brief Builds maxspeed section in mwm with |dataPath|. This section contains max speed limits
/// if it's available.
/// \param maxspeedFilename file name to csv file with maxspeed tag values.
/// \note To start building the section, the following data must be ready:
/// 1. GenerateIntermediateData(). Saves to a file data about maxspeed tags value of road features
/// 2. GenerateFeatures()
/// 3. Generates geometry
void BuildMaxspeed(std::string const & dataPath, std::string const & osmToFeaturePath,
                   std::string const & maxspeedFilename);
}  // namespace routing
