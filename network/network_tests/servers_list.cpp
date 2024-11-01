#include "network/servers_list.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace om::network
{
TEST(ServersList, ParseMetaConfig)
{
  std::optional<MetaConfig> cfg;

  cfg = ParseMetaConfig(R"([ "https://url1/", "https://url2/" ])");
  EXPECT_TRUE(cfg.has_value());
  EXPECT_THAT(cfg->m_serversList, ::testing::ElementsAreArray({"https://url1/", "https://url2/"}));

  cfg = ParseMetaConfig(R"(
    {
      "servers": [ "https://url1/", "https://url2/" ],
      "settings": {
        "key1": "value1",
        "key2": "value2"
      }
    }
  )");
  EXPECT_TRUE(cfg.has_value());
  EXPECT_THAT(cfg->m_serversList, ::testing::ElementsAreArray({"https://url1/", "https://url2/"}));
  EXPECT_EQ(cfg->m_settings.size(), 2);
  EXPECT_EQ(cfg->m_settings["key1"], "value1");
  EXPECT_EQ(cfg->m_settings["key2"], "value2");

  EXPECT_FALSE(ParseMetaConfig(R"(broken json)"));
  EXPECT_FALSE(ParseMetaConfig(R"([])"));
  EXPECT_FALSE(ParseMetaConfig(R"({})"));
  EXPECT_FALSE(ParseMetaConfig(R"({"no_servers": "invalid"})"));
  EXPECT_FALSE(ParseMetaConfig(R"({"servers": "invalid"})"));
}
}  // namespace om::network
