#pragma once

#include <string>

namespace osm
{
/// @name Pure OSM tag value formatters: a raw OSM value in, the value the map stores out.
/// The generator applies them on import (generator/osm2meta.cpp), and anything that has to compare a
/// raw OSM value against what the map shows - the editor, telling a third-party edit from an
/// import-time reformatting - has to reproduce them. An empty result means the value is dropped on
/// import. All are idempotent and share one signature, so that a caller can hold them in a table
/// keyed by the metadata id.
/// @note These bound what the generator stores, not what a user may type: EditableMapObject's
/// validators for the same tags are deliberately stricter (see EditableMapObject::IsValidMetadata,
/// e.g. building levels are 1..50 integers there and 0..167 with fractions here).
//@{
std::string ValidateAndFormat_stars(std::string const & v);
/// @note Not ValidateAndFormat_website() from indexer/validate_and_format_contacts.hpp, which
/// formats the same tag in the opposite direction: that one prepends a scheme to what a user typed,
/// this one strips a trailing slash on import.
std::string ValidateAndFormat_url(std::string const & v);
std::string ValidateAndFormat_internet(std::string const & v);
std::string ValidateAndFormat_height(std::string const & v);
std::string ValidateAndFormat_building_levels(std::string const & v);
std::string ValidateAndFormat_level(std::string const & v);
std::string ValidateAndFormat_drive_through(std::string const & v);
std::string ValidateAndFormat_self_service(std::string const & v);
std::string ValidateAndFormat_outdoor_seating(std::string const & v);
//@}

/// One cuisine token, normalized exactly as the generator does before it looks the classifier type up
/// (generator/osm2type.cpp): lower-cased, with repeated spaces collapsed and every space turned into
/// an underscore, and the duplicate spellings folded ("bbq" and "barbeque" into "barbecue",
/// "doughnut" into "donut", "steak" into "steak_house", "coffee" into "coffee_shop"). The caller
/// splits the tag value on ',' and ';' and drops the tokens that normalize to nothing.
std::string NormalizeCuisineToken(std::string const & v);
}  // namespace osm
