#include "testing/testing.hpp"

#include "local_ads/campaign_serialization.hpp"

#include <random>
#include <vector>

using namespace local_ads;

namespace
{
bool TestSerialization(std::vector<Campaign> const & cs, Version const v)
{
  auto const bytes = Serialize(cs, v);
  return cs == Deserialize(bytes);
}

std::vector<Campaign> GenerateCampaignsV1(size_t number)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> featureIds(1, 600000);
  std::uniform_int_distribution<> icons(1, 4096);
  std::uniform_int_distribution<> expirationDays(1, 30);

  std::vector<Campaign> cs;
  while (number--)
  {
    auto const fid = featureIds(gen);
    auto const iconid = icons(gen);
    auto const days = expirationDays(gen);
    cs.emplace_back(fid, iconid, days);
  }
  return cs;
}

std::vector<Campaign> GenerateCampaignsV2(size_t number)
{
  int kSeed = 42;
  std::mt19937 gen(kSeed);
  std::uniform_int_distribution<> featureIds(1, 600000);
  std::uniform_int_distribution<> icons(1, 4096);
  std::uniform_int_distribution<> expirationDays(1, 30);
  std::uniform_int_distribution<> zoomLevels(10, 17);
  std::uniform_int_distribution<> priorities(0, 7);

  std::vector<Campaign> cs;
  while (number--)
  {
    auto const fid = featureIds(gen);
    auto const iconid = icons(gen);
    auto const days = expirationDays(gen);
    auto const zoom = zoomLevels(gen);
    auto const priority = priorities(gen);
    cs.emplace_back(fid, iconid, days, zoom, priority);
  }
  return cs;
}
}  // namspace

UNIT_TEST(Serialization_Smoke)
{
  TEST(TestSerialization({
        {10, 10, 10},
        {1000, 100, 20},
        {120003, 456, 15}
      }, Version::V1), ());

  TEST(TestSerialization({
        {10, 10, 10, 10, 0},
        {1000, 100, 20, 17, 7},
        {120003, 456, 15, 13, 6}
      }, Version::V2), ());

  TEST(TestSerialization(GenerateCampaignsV1(100), Version::V1), ());
  TEST(TestSerialization(GenerateCampaignsV1(1000), Version::V1), ());
  TEST(TestSerialization(GenerateCampaignsV1(10000), Version::V1), ());

  TEST(TestSerialization(GenerateCampaignsV2(100), Version::V2), ());
  TEST(TestSerialization(GenerateCampaignsV2(1000), Version::V2), ());
  TEST(TestSerialization(GenerateCampaignsV2(10000), Version::V2), ());
}
