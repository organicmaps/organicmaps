#pragma once

#include "platform/safe_callback.hpp"

#include <string>
#include <vector>

namespace storage
{
class Pinger
{
public:
  using Pong = platform::SafeCallback<void(std::vector<std::string> readyUrls)>;
  static void Ping(std::vector<std::string> const & urls, Pong const & pong);
};
}  // namespace storage
