#pragma once

#include "generator/restriction_collector.hpp"
#include "generator/routing_index_generator.hpp"

#include <memory>
#include <string>

namespace routing_builder
{
std::unique_ptr<routing::IndexGraph> CreateIndexGraph(std::string const & mwmPath, std::string const & country,
                                                      CountryParentNameGetterFn const & countryParentNameGetterFn);

void SerializeRestrictions(RestrictionCollector & restrictionCollector, std::string const & mwmPath);
// This function is the generator tool's interface to building the mwm
// section which contains road restrictions. (See https://wiki.openstreetmap.org/wiki/Restriction)
// As long as the restrictions are built later than the road features themselves
// during the generation process, we have to store a mapping between osm ids and feature ids:
// the restrictions are written in OSM terms while for the road features only their feature ids
// are known.
// This results in the following pipeline:
//   -- OsmToFeatureTranslator uses RestrictionsWriter to prepare a .csv file
//      with restrictions in terms of OSM ids while parsing the OSM source.
//   -- OsmID2FeatureID saves the OSM id -> feature id mapping.
//   -- After all features have been created, RestrictionCollector reads the csv
//      and builds the mwm section with restrictions. The serialization/deserialization
//      code is in the routing library.

/// \brief Builds section with road restrictions.
/// \param mwmPath path to mwm which will be added with road restriction section.
/// \param restrictionPath comma separated (csv like) file with road restrictions in osm ids terms
/// in the following format:
/// <type of restrictions>, <osm id 1 of the restriction>, <osm id 2>, and so on
/// For example:
/// Only, 335049632, 49356687,
/// No, 157616940, 157616940,
/// \param osmIdsToFeatureIdsPath a binary file with mapping form osm ids to feature ids.
/// One osm id is mapped to one feature id. The file should be saved with the help of
/// OsmID2FeatureID class or using a similar way.
bool BuildRoadRestrictions(routing::IndexGraph & graph, std::string const & mwmPath,
                           std::string const & restrictionPath, std::string const & osmIdsToFeatureIdsPath);
}  // namespace routing_builder
