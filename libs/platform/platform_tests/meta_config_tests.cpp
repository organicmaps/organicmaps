#include "testing/testing.hpp"

#include "platform/products.hpp"
#include "platform/servers_list.hpp"

namespace meta_config_tests
{
using downloader::ParseMetaConfig;

UNIT_TEST(MetaConfig_JSONParser_InvalidConfig)
{
  for (auto const json :
       {R"({"servers": ["http://url1", "http://url2")", "[]", "{}", R"({"servers": []})",
        R"({"no_servers": "invalid"})", R"({"servers": "invalid"})", R"({"settings": {"key1": "value1"}})"})
  {
    TEST(!ParseMetaConfig(json), (json));
  }
}

UNIT_TEST(MetaConfig_JSONParser_ServersOnly)
{
  auto const metaConfig = ParseMetaConfig(R"({"servers": ["http://url1", "http://url2"]})");
  TEST(metaConfig, ());
  TEST_EQUAL(metaConfig->servers, (downloader::MetaConfig::ServersList{"http://url1", "http://url2"}), ());
  TEST(metaConfig->settings.empty(), ());
  TEST(!metaConfig->productsConfig, ());
}

UNIT_TEST(MetaConfig_JSONParser_Settings)
{
  std::string_view constexpr json = R"({
    "servers": ["http://url1", "http://url2"],
    "settings": {
      "DonateUrl": "value1",
      "NY": "false",
      "key2": "value2"
    }
  })";
  auto const metaConfig = ParseMetaConfig(json);
  TEST(metaConfig, ());
  TEST_EQUAL(metaConfig->servers, (downloader::MetaConfig::ServersList{"http://url1", "http://url2"}), ());
  TEST_EQUAL(metaConfig->settings,
             (downloader::MetaConfig::SettingsMap{{"DonateUrl", "value1"}, {"NY", "false"}, {"key2", "value2"}}), ());
  TEST(!metaConfig->productsConfig, ());
  TEST_EQUAL(metaConfig->terrainServers, (downloader::MetaConfig::ServersList{"https://cdn.organicmaps.app/"}), ());
}

UNIT_TEST(MetaConfig_JSONParser_EmptyTerrainServersUseDefault)
{
  auto const config = ParseMetaConfig(R"({"servers":["https://maps.example/"],"terrainServers":[]})");
  TEST(config, ());
  TEST_EQUAL(config->terrainServers, downloader::MetaConfig{}.terrainServers, ());
}

UNIT_TEST(MetaConfig_JSONParser_TerrainServers)
{
  auto const metaConfig = ParseMetaConfig(R"({
    "servers": ["https://maps.example/"],
    "terrainServers": ["https://terrain1.example/", "https://terrain2.example/"]
  })");
  TEST(metaConfig, ());
  TEST_EQUAL(metaConfig->servers, (downloader::MetaConfig::ServersList{"https://maps.example/"}), ());
  TEST_EQUAL(metaConfig->terrainServers,
             (downloader::MetaConfig::ServersList{"https://terrain1.example/", "https://terrain2.example/"}), ());
  TEST(!ParseMetaConfig(R"({"servers": ["https://maps.example/"], "terrainServers": "invalid"})"), ());
}

UNIT_TEST(MetaConfig_JSONParser_UnknownKeys)
{
  auto const metaConfig = ParseMetaConfig(R"({
    "servers": ["https://maps.example/"],
    "futureServers": ["https://future.example/"],
    "futureSettings": {"key": [1, 2]},
    "productsConfig": {
      "placePagePrompt": "prompt",
      "futurePrompt": {"text": "new"},
      "products": [{"title": "Product", "link": "https://product.example/", "futureSubtitle": "new"}]
    }
  })");
  TEST(metaConfig, ("A newer meta server field must not break the reply"));
  TEST_EQUAL(metaConfig->servers, (downloader::MetaConfig::ServersList{"https://maps.example/"}), ());

  TEST(metaConfig->productsConfig, ());
  auto const & productsConfig = *metaConfig->productsConfig;
  TEST_EQUAL(productsConfig.placePagePrompt, "prompt", ());
  TEST_EQUAL(productsConfig.products.size(), 1, ());
  TEST_EQUAL(productsConfig.products[0].title, "Product", ());
  TEST_EQUAL(productsConfig.products[0].link, "https://product.example/", ());
}

UNIT_TEST(MetaConfig_JSONParser_InvalidProducts)
{
  for (auto const productsJson :
       {R"({})", R"({"products":[]})", R"({"products":[{"title":"Product"}]})",
        R"({"products":[{"link":"https://product.example/"}]})",
        R"({"products":[{"title":"","link":"https://product.example/"}]})",
        R"({"products":[{"title":"Product","link":""}]})",
        R"({"products":[{"title":"Product","link":"https://product.example/"},{"title":"Invalid"}]})"})
  {
    auto const json = std::string{R"({"servers":["https://maps.example/"],
      "settings":{"DonateUrl":"https://donate.example/"},"productsConfig":)"} +
                      productsJson + "}";
    auto const metaConfig = ParseMetaConfig(json);
    TEST(metaConfig, (json));
    TEST_EQUAL(metaConfig->servers, (downloader::MetaConfig::ServersList{"https://maps.example/"}), (json));
    TEST_EQUAL(metaConfig->settings, (downloader::MetaConfig::SettingsMap{{"DonateUrl", "https://donate.example/"}}),
               (json));
    TEST(!metaConfig->productsConfig, (json));
  }
}

UNIT_TEST(MetaConfig_JSONParser_Products)
{
  std::string_view constexpr json = R"({
    "servers": ["http://url1"],
    "settings": {
      "DonateUrl": "value1"
    },
    "productsConfig": {
      "placePagePrompt": "prompt1",
      "products": [
        {
          "title": "Product 1",
          "link": "http://product1"
        },
        {
          "title": "Product 2",
          "link": "http://product2"
        }
      ]
    }
  })";

  auto const metaConfig = ParseMetaConfig(json);
  TEST(metaConfig, ());
  TEST_EQUAL(metaConfig->settings, (downloader::MetaConfig::SettingsMap{{"DonateUrl", "value1"}}), ());
  TEST(metaConfig->productsConfig, ());
  auto const & productsConfig = *metaConfig->productsConfig;
  TEST_EQUAL(productsConfig.placePagePrompt, "prompt1", ());
  TEST_EQUAL(productsConfig.products.size(), 2, ());
  TEST_EQUAL(productsConfig.products[0].title, "Product 1", ());
  TEST_EQUAL(productsConfig.products[0].link, "http://product1", ());
  TEST_EQUAL(productsConfig.products[1].title, "Product 2", ());
  TEST_EQUAL(productsConfig.products[1].link, "http://product2", ());
}

}  // namespace meta_config_tests
