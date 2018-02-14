#pragma once

#include <limits>
#include <set>
#include <vector>

namespace df
{
using MarkID = uint32_t;
using LineID = uint32_t;
using MarkGroupID = uint32_t;
using MarkIDCollection = std::vector<MarkID>;
using LineIDCollection = std::vector<LineID>;
using MarkIDSet = std::set<MarkID, std::greater<MarkID>>;
using LineIDSet = std::set<LineID>;
using GroupIDCollection = std::vector<MarkGroupID>;
using GroupIDSet = std::set<MarkGroupID>;

MarkID const kInvalidMarkId = std::numeric_limits<MarkID>::max();
LineID const kInvalidLineId = std::numeric_limits<LineID>::max();
MarkGroupID const kInvalidMarkGroupId = std::numeric_limits<MarkGroupID>::max();
}  // namespace df
