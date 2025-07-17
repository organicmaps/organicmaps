#pragma once

#include "platform/safe_callback.hpp"

#include <string>
#include <vector>

namespace storage
{
class Pinger
{
public:
  using Endpoints = std::vector<std::string>;
  // Returns list of available endpoints. Works synchronously.
  static Endpoints ExcludeUnavailableAndSortEndpoints(Endpoints const & urls);
};
}  // namespace storage
