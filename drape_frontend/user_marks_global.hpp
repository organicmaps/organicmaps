#pragma once

#include <set>
#include <vector>

namespace df
{
using MarkID = uint32_t;
using MarkGroupID = size_t;
using IDCollection = std::vector<MarkID>;
using MarkIDSet = std::set<MarkID>;
using GroupIDList = std::vector<MarkGroupID>;
using GroupIDSet = std::set<MarkGroupID>;
}  // namespace df
