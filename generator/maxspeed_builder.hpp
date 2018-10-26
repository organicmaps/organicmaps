#pragma once
#include "routing/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include "base/geo_object_id.hpp"

#include <map>
#include <string>
#include <vector>

namespace routing
{
struct ParsedMaxspeed
{
  measurement_utils::Units m_units = measurement_utils::Units::Metric;
  uint16_t m_forward = routing::kInvalidSpeed;
  uint16_t m_backward = routing::kInvalidSpeed;
};

using OsmIdToMaxspeed = std::map<base::GeoObjectId, ParsedMaxspeed>;

bool ParseMaxspeeds(std::string const & maxspeedFilename, OsmIdToMaxspeed & osmIdToMaxspeed);

/// \brief Write |speeds| to maxspeed section to mwm with |dataPath|.
void SerializeMaxspeed(std::string const & dataPath, std::vector<FeatureMaxspeed> && speeds);

/// \brief Builds maxspeed section in mwm with |dataPath|. This section contains max speed limits
/// if it's available.
/// \param maxspeedFilename file name to csv file with maxspeed tag values.
/// \note To start building the section, the following data must be ready:
/// 1. GenerateIntermediateData(). Saves to a file data about maxspeed tags value of road features
/// 2. GenerateFeatures()
/// 3. Generates geometry
bool BuildMaxspeed(std::string const & dataPath, std::string const & osmToFeaturePath,
                   std::string const & maxspeedFilename);
}  // namespace routing
