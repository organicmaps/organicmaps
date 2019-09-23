#pragma once

#include <string>

namespace opentable
{
class Api
{
public:
  static std::string GetBookTableUrl(std::string const & restaurantId);
};
}  // namespace opentable
