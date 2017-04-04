#pragma once

#include "local_ads/campaign.hpp"

#include <cstdint>
#include <vector>

namespace local_ads
{
std::vector<uint8_t> Serialize(std::vector<Campaign> const & campaigns);
std::vector<Campaign> Deserialize(std::vector<uint8_t> const & bytes);
}  // namespace local_ads
