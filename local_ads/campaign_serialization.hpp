#pragma once

#include "local_ads/campaign.hpp"

#include <cstdint>
#include <vector>

namespace local_ads
{
enum class Version
{
  unknown = -1,
  v1 = 0,  // March 2017 (Store feature ids and icon ids as varints,
           // use one byte for days before expiration.)
  latest = v1
};

std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns);
std::vector<Campaign> Deserialize(std::vector<uint8_t> const & bytes);
}  // namespace local_ads
