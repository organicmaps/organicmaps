#include "testing/testing.hpp"

#include "local_ads/campaign_serialization.hpp"

#include <limits>
#include <random>
#include <vector>

using namespace local_ads;

namespace
{
template <typename T>
using Limits = typename std::numeric_limits<T>;

bool TestSerialization(std::vector<Campaign> const & cs, Version const v)
{
  auto const bytes = Serialize(cs, v);
  return cs == Deserialize(bytes);
}

std::vector<Campaign> GenerateCampaignsV1(size_t number)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> featureIds(1, Limits<uint32_t>::max());
  std::uniform_int_distribution<> icons(1, Limits<uint16_t>::max());
  std::uniform_int_distribution<> expirationDays(1, Limits<uint8_t>::max());

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
  std::uniform_int_distribution<uint32_t> featureIds(1, Limits<uint32_t>::max());
  std::uniform_int_distribution<> icons(1, Limits<uint16_t>::max());
  std::uniform_int_distribution<> expirationDays(1, Limits<uint8_t>::max());
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
        {0, 0, 0},
        {Limits<uint32_t>::max(), Limits<uint16_t>::max(), Limits<uint8_t>::max()},
        {120003, 456, 15}
      }, Version::V1), ());

  TEST(TestSerialization({
        {0, 0, 0, 10, 0},
        {Limits<uint32_t>::max(), Limits<uint16_t>::max(), Limits<uint8_t>::max()},
        {1000, 100, 255, 17, 7},
        {120003, 456, 15, 13, 6}
      }, Version::V2), ());

  TEST(TestSerialization({
         {241925, 6022, 255, 16, 6},
         {241927, 6036, 255, 16, 0},
         {164169, 3004, 255, 15, 0},
         {164172, 3001, 255, 15, 7}
       }, Version::V2), ());

  TEST(TestSerialization(GenerateCampaignsV1(100), Version::V1), ());
  TEST(TestSerialization(GenerateCampaignsV1(1000), Version::V1), ());
  TEST(TestSerialization(GenerateCampaignsV1(10000), Version::V1), ());

  TEST(TestSerialization(GenerateCampaignsV2(100), Version::V2), ());
  TEST(TestSerialization(GenerateCampaignsV2(1000), Version::V2), ());
  TEST(TestSerialization(GenerateCampaignsV2(10000), Version::V2), ());
}
