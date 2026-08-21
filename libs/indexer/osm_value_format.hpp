#pragma once

#include <string>

namespace osm
{
/// @name Pure OSM tag value formatters, shared by the map generator (generator/osm2meta.cpp) and by
/// the editor, which has to map a raw OSM value into the MWM value domain to tell a third-party
/// edit from an import-time reformatting (editor/osm_tag_policy.hpp).
/// An empty result means the value is dropped on import. All of them are idempotent, and they share
/// one signature so that editor/osm_tag_policy.cpp can hold them in a table.
//@{
std::string ValidateAndFormat_stars(std::string const & v);
std::string ValidateAndFormat_url(std::string const & v);
std::string ValidateAndFormat_internet(std::string const & value);
std::string ValidateAndFormat_height(std::string const & v);
std::string ValidateAndFormat_building_levels(std::string const & value);
std::string ValidateAndFormat_level(std::string const & value);
std::string ValidateAndFormat_drive_through(std::string const & value);
std::string ValidateAndFormat_self_service(std::string const & value);
std::string ValidateAndFormat_outdoor_seating(std::string const & value);
//@}

/// One cuisine token, normalized exactly as the generator does before it looks the classifier type up
/// (generator/osm2type.cpp): lower-cased, with repeated spaces collapsed and every space turned into
/// an underscore, and the duplicate spellings folded ("bbq" and "barbeque" into "barbecue",
/// "doughnut" into "donut", "steak" into "steak_house", "coffee" into "coffee_shop"). The caller
/// splits the tag value on ',' and ';' and drops the tokens that normalize to nothing.
std::string NormalizeCuisineToken(std::string const & v);
}  // namespace osm
