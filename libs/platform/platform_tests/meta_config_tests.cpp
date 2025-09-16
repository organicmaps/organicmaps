#include "testing/testing.hpp"

#include "platform/products.hpp"
#include "platform/servers_list.hpp"

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
  TEST(productsConfig.HasProducts(), ());

  TEST_EQUAL(productsConfig.products.size(), 2, ());
}

}  // namespace meta_config_tests
