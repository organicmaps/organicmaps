#pragma once

#include <string>

#include "map_object.hpp"

namespace osm
{
std::string ValidateAndFormat_website(std::string const & v);
std::string ValidateAndFormat_facebook(std::string const & v);
std::string ValidateAndFormat_instagram(std::string const & v);
std::string ValidateAndFormat_twitter(std::string const & v);
std::string ValidateAndFormat_vk(std::string const & v);
std::string ValidateAndFormat_contactLine(std::string const & v);
std::string ValidateAndFormat_fediverse(std::string const & v);
std::string ValidateAndFormat_bluesky(std::string const & v);

bool ValidateWebsite(std::string const & site);
bool ValidateFacebookPage(std::string const & v);
bool ValidateInstagramPage(std::string const & v);
bool ValidateTwitterPage(std::string const & v);
bool ValidateVkPage(std::string const & v);
bool ValidateLinePage(std::string const & v);
bool ValidateFediversePage(std::string const & v);
bool ValidateBlueskyPage(std::string const & v);

bool isSocialContactTag(std::string_view tag);
bool isSocialContactTag(osm::MapObject::MetadataID const metaID);
std::string socialContactToURL(std::string_view tag, std::string_view value);
std::string socialContactToURL(osm::MapObject::MetadataID metaID, std::string_view value);
}  // namespace osm
