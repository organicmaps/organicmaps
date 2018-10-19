#pragma once

#include <string>

namespace generator
{
/// \brief Builds maxspeed section.
/// \param maxspeedFilename file name to csv file with maxspeed tag values.
/// \note To start building the section, the following data must be ready:
/// 1. GenerateIntermediateData(). Saves to a file data about maxspeed tags value of road features
/// 2. GenerateFeatures()
/// 3. Generates geometry
bool BuildMaxspeed(std::string const & dataPath, std::string const & osmToFeaturePath,
                   std::string const & maxspeedFilename);
}  // namespace generator
