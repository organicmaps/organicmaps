#pragma once

#include "transit/transit_graph_data.hpp"

#include "storage/storage_defines.hpp"

#include <string>

namespace routing
{
namespace transit
{
/// \brief Fills |data| according to a transit graph (|transitJsonPath|).
/// \note Some fields of |data| contain feature ids of a certain mwm. These fields are filled
/// iff the mapping (|osmIdToFeatureIdsPath|) contains them. Otherwise the fields have default value.
void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping, std::string const & transitJsonPath, GraphData & data);

/// \brief Calculates and adds some information to transit graph (|data|) after deserializing
/// from json.
void ProcessGraph(std::string const & mwmPath, storage::CountryId const & countryId,
                  OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, GraphData & data);

/// \brief Builds the transit section in the mwm based on transit graph in json which represents
/// transit graph clipped by the mwm borders.
/// \param mwmDir relative or full path to a directory where mwm is located.
/// \param countryId is an mwm name without extension of the processed mwm.
/// \param osmIdToFeatureIdsPath is a path to a file with osm id to feature ids mapping.
/// \param transitDir relative or full path to a directory with json files of transit graphs.
/// It's assumed that the files have the same name with country ids and extension TRANSIT_FILE_EXTENSION.
/// \note An mwm pointed by |mwmPath| should contain:
/// * feature geometry
/// * index graph (ROUTING_FILE_TAG)
void BuildTransit(std::string const & mwmDir, storage::CountryId const & countryId,
                  std::string const & osmIdToFeatureIdsPath, std::string const & transitDir);
}  // namespace transit
}  // namespace routing
