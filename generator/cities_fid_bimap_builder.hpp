#pragma once

#include "indexer/fid_bimap.hpp"

#include <string>

namespace generator
{
// Takes the FeatureID <-> base::GeoObjectId bidirectional map for features
// corresponding to cities from |osmToFeaturePath| and serializes it
// as a section in the mwm file at |dataPath|.
bool BuildCitiesFidBimap(std::string const & dataPath, std::string const & osmToFeaturePath);
}  // namespace generator
