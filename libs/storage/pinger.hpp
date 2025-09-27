#pragma once

#include "platform/safe_callback.hpp"

#include <string>
#include <vector>

namespace storage
{
auto constexpr kDefaultTimeoutInSeconds = 4.0;

class Pinger
{
public:
  using Endpoints = std::vector<std::string>;
  // Returns list of available endpoints. Works synchronously.
  static Endpoints ExcludeUnavailableAndSortEndpoints(Endpoints const & urls,
                                                      int64_t const timeoutInSeconds = kDefaultTimeoutInSeconds);
};
}  // namespace storage
