#pragma once

#include "indexer/map_style.hpp"

#include "platform/settings.hpp"

#include <string_view>

namespace sailfish
{
template <typename T>
T LoadSetting(std::string_view key, T defaultValue)
{
  T value;
  return settings::Get(key, value) ? value : defaultValue;
}

// Without the vehicle style used while navigating.
MapStyle BaseMapStyle(bool dark, bool outdoors);
}  // namespace sailfish
