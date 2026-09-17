#pragma once

#include <string>

#include "map_object.hpp"

namespace osm
{
/// Prepends a scheme to a website a user typed. @note Not ValidateAndFormat_url() from
/// indexer/osm_value_format.hpp, which formats the same tag in the opposite direction: that one
/// strips a trailing slash off a raw OSM value on import.
std::string ValidateAndFormat_website(std::string const & v);
std::string ValidateAndFormat_facebook(std::string const & v);
std::string ValidateAndFormat_instagram(std::string const & v);
std::string ValidateAndFormat_twitter(std::string const & v);
std::string ValidateAndFormat_vk(std::string const & v);
std::string ValidateAndFormat_contactLine(std::string const & v);

bool ValidateWebsite(std::string const & site);
bool ValidateFacebookPage(std::string const & v);
bool ValidateInstagramPage(std::string const & v);
bool ValidateTwitterPage(std::string const & v);
bool ValidateVkPage(std::string const & v);
bool ValidateLinePage(std::string const & v);

bool isSocialContactTag(std::string_view tag);
bool isSocialContactTag(osm::MapObject::MetadataID const metaID);
std::string socialContactToURL(std::string_view tag, std::string_view value);
std::string socialContactToURL(osm::MapObject::MetadataID metaID, std::string_view value);
}  // namespace osm
