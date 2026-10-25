#include "platform/servers_list.hpp"

#include "base/logging.hpp"

#include "glaze/json/read.hpp"

namespace downloader
{

std::optional<MetaConfig> ParseMetaConfig(std::string_view jsonStr)
{
  MetaConfig outMetaConfig;

  // Ignore unknown fields so this client tolerates additive meta server changes.
  // Older clients should never receive new fields.
  glz::opts constexpr opts{.error_on_unknown_keys = false};
  if (auto const ec = glz::read<opts>(outMetaConfig, jsonStr); ec)
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

  // TODO(AB): Return  settings/product configs or insert fallback servers?
  if (outMetaConfig.servers.empty())
    return std::nullopt;

  return outMetaConfig;
}
}  // namespace downloader
