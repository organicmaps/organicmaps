#pragma once

#include <string>

namespace osm {
  std::string ValidateAndFormat_facebook(std::string const & v);
  std::string ValidateAndFormat_instagram(std::string const & v);
  std::string ValidateAndFormat_twitter(std::string const & v);
  std::string ValidateAndFormat_vk(std::string const & v);
  std::string ValidateAndFormat_contactLine(std::string const & v);
}