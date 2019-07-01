#pragma once

#include <string>

namespace generator
{
// Takes the FeatureID <-> base::GeoObjectId bidirectional map for features
// corresponding to cities from |osmToFeaturePath| and serializes it
// as a section in the mwm file at |dataPath|.
// Only works for World.mwm and always returns false otherwise.
bool BuildCitiesIds(std::string const & dataPath, std::string const & osmToFeaturePath);

// Takes the FeatureID <-> base::GeoObjectId bidirectional map for features
// corresponding to cities from the (test) features metadata and serializes the map
// as a section in the mwm file at |dataPath|.
// Only works for World.mwm and always returns false otherwise.
bool BuildCitiesIdsForTesting(std::string const & dataPath);
}  // namespace generator
