#include "platform/servers_list.hpp"

#include "platform/http_request.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "cppjansson/cppjansson.hpp"

namespace downloader
{

std::optional<MetaConfig> ParseMetaConfig(std::string const & jsonStr)
{
  char const kSettings[] = "settings";
  char const kServers[] = "servers";
  char const kProductsConfig[] = "productsConfig";

  MetaConfig outMetaConfig;
  try
  {
    base::Json const root(jsonStr.c_str());
    json_t const * servers;
    if (json_is_object(root.get()))
    {
      // New format:
      // {
      //   "servers": ["http://url1", "http://url2", "http://url3"],
      //   "settings": {
      //      "key1": "value1",
      //      "key2": "http://url"
      //    }
      // }

      json_t * settings = json_object_get(root.get(), kSettings);
      char const * key;
      json_t const * value;
      json_object_foreach(settings, key, value)
      {
        if (key == settings::kDonateUrl || key == settings::kNY)
        {
          char const * valueStr = json_string_value(value);
          if (value)
            outMetaConfig.m_settings[key] = valueStr;
        }
      }

      servers = json_object_get(root.get(), kServers);

      auto const productsConfig = json_object_get(root.get(), kProductsConfig);
      if (productsConfig)
        outMetaConfig.m_productsConfig = json_dumps(productsConfig, JSON_ENCODE_ANY);
      else
        LOG(LINFO, ("No ProductsConfig in meta configuration"));
    }
    else
    {
      // Old format:
      // ["http://url1", "http://url2", "http://url3"]
      servers = root.get();
    }

    for (size_t i = 0; i < json_array_size(servers); ++i)
    {
      char const * url = json_string_value(json_array_get(servers, i));
      if (url)
        outMetaConfig.m_serversList.push_back(url);
    }
  }
  catch (base::Json::Exception const & ex)
  {
    LOG(LWARNING, ("Can't parse meta configuration:", ex.Msg(), jsonStr));
  }

  if (outMetaConfig.m_serversList.empty())
    return std::nullopt;

  return outMetaConfig;
}
}  // namespace downloader
