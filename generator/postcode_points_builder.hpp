#pragma once

#include <string>

namespace storage
{
class CountryInfoGetter;
}

namespace indexer
{
enum class PostcodePointsDatasetType
{
  UK,
  US
};

// Builds postcodes section with external postcodes data and writes it to the mwm file.
bool BuildPostcodePoints(std::string const & path, std::string const & country, PostcodePointsDatasetType type,
                         std::string const & datasetPath, bool forceRebuild);
// Exposed for testing.
bool BuildPostcodePointsWithInfoGetter(std::string const & path, std::string const & country,
                                       PostcodePointsDatasetType type, std::string const & datasetPath,
                                       bool forceRebuild, storage::CountryInfoGetter const & infoGetter);
}  // namespace indexer
