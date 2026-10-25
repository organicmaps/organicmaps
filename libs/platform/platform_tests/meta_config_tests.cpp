#include "testing/testing.hpp"

#include "platform/products.hpp"
#include "platform/servers_list.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"

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

#ifdef DEBUG
UNIT_TEST(MetaConfig_JSONParser_InvalidProducts)
{
  static bool assertionRaised;
  auto const previousAssert = base::SetAssertFunction([](base::SrcPoint const &, std::string const &) -> bool
  {
    assertionRaised = true;
    return false;
  });
  SCOPE_GUARD(restoreAssert, [previousAssert] { base::SetAssertFunction(previousAssert); });

  for (
      auto const json : {
          R"({"servers":["https://maps.example/"],"productsConfig":{"products":[]}})",
          R"({"servers":["https://maps.example/"],"productsConfig":{"products":[{"title":"Product"}]}})",
          R"({"servers":["https://maps.example/"],"productsConfig":{"products":[{"link":"https://product.example/"}]}})",
      })
  {
    assertionRaised = false;
    ParseMetaConfig(json);
    TEST(assertionRaised, (json));
  }
}
#endif

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
