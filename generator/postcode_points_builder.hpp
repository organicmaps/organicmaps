#pragma once

#include <string>

namespace storage
{
class CountryInfoGetter;
}

namespace indexer
{
// Builds postcodes section with external postcodes data and writes it to the mwm file.
bool BuildPostcodePoints(std::string const & path, std::string const & country,
                         std::string const & ukDatasetPath, std::string const & usDatasetPath,
                         bool forceRebuild);
// Exposed for testing.
bool BuildPostcodePointsWithInfoGetter(std::string const & path, std::string const & country,
                                       std::string const & ukDatasetPath,
                                       std::string const & usDatasetPath, bool forceRebuild,
                                       storage::CountryInfoGetter const & infoGetter);
}  // namespace indexer
