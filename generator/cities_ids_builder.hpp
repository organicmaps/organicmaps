#pragma once

#include <string>

namespace generator
{
// Takes the FeatureID <-> base::GeoObjectId bidirectional map for features
// corresponding to cities from |osmToFeaturePath| and serializes it
// as a section in the mwm file at |dataPath|.
// For World.mwm returns true iff the section was successfully built.
// For non-world mwms returns false.
bool BuildCitiesIds(std::string const & dataPath, std::string const & osmToFeaturePath);

// Takes the FeatureID <-> base::GeoObjectId bidirectional map for features
// corresponding to cities from the (test) features metadata and serializes the map
// as a section in the mwm file at |dataPath|.
// For World.mwm returns true iff the section was successfully built.
// For non-world mwms returns false.
bool BuildCitiesIdsForTesting(std::string const & dataPath);
}  // namespace generator
