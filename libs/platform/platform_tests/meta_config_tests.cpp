#include "testing/testing.hpp"

#include "platform/products.hpp"
#include "platform/servers_list.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"

namespace meta_config_tests
{
using downloader::ParseMetaConfig;

UNIT_TEST(MetaConfig_JSONParser_InvalidJSON)
{
  TEST(!ParseMetaConfig(R"({"servers": ["http://url1", "http://url2")"), ());
}

UNIT_TEST(MetaConfig_JSONParser_EmptyServersList)
{
  TEST(!ParseMetaConfig(R"({"servers": []})"), ());
}

UNIT_TEST(MetaConfig_JSONParser_MissingRequiredServersKey)
{
  std::string_view constexpr json = R"({
    "settings": {
      "key1": "value1"
    }
  })";
  TEST(!ParseMetaConfig(json), ("JSON shouldn't be parsed without a required servers key"));
}

UNIT_TEST(MetaConfig_JSONParser_NewFormatWithoutProducts)
{
  std::string_view constexpr newFormatJson = R"({
    "servers": ["http://url1", "http://url2"],
    "settings": {
      "DonateUrl": "value1",
      "key2": "value2"
    }
  })";
  auto const metaConfig = ParseMetaConfig(newFormatJson);
  TEST(metaConfig, ());
  TEST_EQUAL(metaConfig->servers.size(), 2, ());
  TEST_EQUAL(metaConfig->servers[0], "http://url1", ());
  TEST_EQUAL(metaConfig->servers[1], "http://url2", ());

  TEST_EQUAL(metaConfig->settings.size(), 2, ());

  auto found = metaConfig->settings.find("DonateUrl");
  TEST(found != metaConfig->settings.end(), ());
  TEST_EQUAL(found->second, "value1", ());

  found = metaConfig->settings.find("key2");
  TEST(found != metaConfig->settings.end(), ());
  TEST_EQUAL(found->second, "value2", ());

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

UNIT_TEST(MetaConfig_JSONParser_NewFormatWithProducts)
{
  std::string_view constexpr newFormatJson = R"({
    "servers": ["http://url1", "http://url2"],
    "settings": {
      "DonateUrl": "value1",
      "key2": "value2"
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

  auto const metaConfig = ParseMetaConfig(newFormatJson);
  TEST(metaConfig, ());
  TEST_EQUAL(metaConfig->servers.size(), 2, ());
  TEST_EQUAL(metaConfig->servers[0], "http://url1", ());
  TEST_EQUAL(metaConfig->servers[1], "http://url2", ());

  TEST_EQUAL(metaConfig->settings.size(), 2, ());

  auto found = metaConfig->settings.find("DonateUrl");
  TEST(found != metaConfig->settings.end(), ());
  TEST_EQUAL(found->second, "value1", ());
  found = metaConfig->settings.find("key2");
  TEST(found != metaConfig->settings.end(), ());
  TEST_EQUAL(found->second, "value2", ());

  TEST(metaConfig->productsConfig, ());
  auto const & productsConfig = *metaConfig->productsConfig;
  TEST_EQUAL(productsConfig.placePagePrompt, "prompt1", ());
  TEST_EQUAL(productsConfig.products.size(), 2, ());
}

}  // namespace meta_config_tests
