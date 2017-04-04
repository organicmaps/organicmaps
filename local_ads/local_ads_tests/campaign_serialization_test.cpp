#include "testing/testing.hpp"

#include "local_ads/campaign_serialization.hpp"

#include <random>
#include <vector>

using namespace local_ads;

namespace
{
bool TestSerialization(std::vector<Campaign> const & cs)
{
  auto const bytes = Serialize(cs);
  return cs == Deserialize(bytes);
}

std::vector<Campaign> GenerateRandomCampaigns(size_t number)
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
    cs.emplace_back(fid, iconid, days, false /* priorityBit */);
  }
  return cs;
}
}  // namspace

UNIT_TEST(Serialization_Smoke)
{
  TEST(TestSerialization({
        {10, 10, 10, 0},
        {1000, 100, 20, 0},
        {120003, 456, 15, 0}
      }), ());

  TEST(TestSerialization(GenerateRandomCampaigns(100)), ());
  TEST(TestSerialization(GenerateRandomCampaigns(1000)), ());
  TEST(TestSerialization(GenerateRandomCampaigns(10000)), ());
}
