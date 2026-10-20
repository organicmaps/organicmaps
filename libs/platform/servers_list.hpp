#pragma once

#include "platform/products.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace downloader
{
// Dynamic configuration from MetaServer.
// Struct member variable names exactly match json fields.
struct MetaConfig
{
  using ServersList = std::vector<std::string>;
  ServersList servers;
  // Older meta servers do not advertise terrain mirrors.
  ServersList terrainServers = {"https://cdn.organicmaps.app/"};
  using SettingsMap = std::map<std::string, std::string>;
  SettingsMap settings;
  std::optional<products::ProductsConfig> productsConfig;
};

std::optional<MetaConfig> ParseMetaConfig(std::string_view jsonStr);
}  // namespace downloader
