#pragma once

#include "local_ads/campaign.hpp"

#include <cstdint>
#include <vector>

namespace local_ads
{
enum class Version
{
  unknown = -1,
  // March 2017 (store feature ids and icon ids as varints, use one byte for days before
  // expiration).
  v1 = 0,
  // August 2017 (store zoom level and priority as 0-7 values in one byte).
  v2 = 1,

  latest = v2
};
std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns, Version const version);
std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns);
std::vector<Campaign> Deserialize(std::vector<uint8_t> const & bytes);

std::string DebugPrint(local_ads::Version version);
}  // namespace local_ads
