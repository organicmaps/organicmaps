#pragma once

#include "storage/storage_defines.hpp"

#include "transit/experimental/transit_data.hpp"

#include <string>

namespace transit
{
namespace experimental
{
/// \brief Fills |data| according to transit files (|transitJsonPath|).
/// \note Stops and gates items may contain osm_id field. If it is present and the mapping
/// |osmIdToFeatureIdsPath| contains it then its value is used as id. Otherwise generated on the
/// previous step (gtfs_converter) id is used.
void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping, std::string const & transitJsonsPath,
                         TransitData & data);

/// \brief Builds transit section in the mwm based on transit data in json which is already clipped
/// by the mwm borders.
/// \param mwmDir is the directory where mwm is located.
/// \param countryId is an mwm name without extension of the processed mwm.
/// \param osmIdToFeatureIdsPath is a path to a file with osm id to feature ids mapping.
/// \param transitDir is directory with transit jsons.
/// It's assumed that files have extension TRANSIT_FILE_EXTENSION.
/// \note An mwm pointed by |mwmPath| should contain:
/// * feature geometry
/// * index graph (ROUTING_FILE_TAG)
EdgeIdToFeatureId BuildTransit(std::string const & mwmDir, storage::CountryId const & countryId,
                               std::string const & osmIdToFeatureIdsPath, std::string const & transitDir);
}  // namespace experimental
}  // namespace transit
