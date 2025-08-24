#include "platform/servers_list.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "glaze/json.hpp"

namespace downloader
{

std::optional<MetaConfig> ParseMetaConfig(std::string_view jsonStr)
{
  MetaConfig outMetaConfig;

  if (auto const ec = glz::read_json(outMetaConfig, jsonStr); ec)
  {
#ifdef DEBUG
    constexpr auto level = LWARNING;
#else
    auto const level = LERROR;
#endif
    LOG(level, ("Error parsing config", jsonStr, ec));
    return {};
  }

  if (!outMetaConfig.productsConfig)
    LOG(LINFO, ("No ProductsConfig in meta configuration"));

  // TODO(AB): Remove.
  LOG(LDEBUG, ("Servers in config:", outMetaConfig.servers));
  LOG(LDEBUG, ("Settings in config:", outMetaConfig.settings));

  // TODO(AB): Return  settings/product configs or insert fallback servers?
  if (outMetaConfig.servers.empty())
    return std::nullopt;

  return outMetaConfig;
}
}  // namespace downloader
