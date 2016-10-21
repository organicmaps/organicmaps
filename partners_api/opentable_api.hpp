#pragma once

#include "std/string.hpp"

namespace opentable
{
class Api
{
public:
  static string GetBookTableUrl(string const & restaurantId);
};
}  // namespace opentable
