#pragma once

#include <set>
#include <vector>

namespace df
{
using MarkID = uint32_t;
using MarkGroupID = size_t;
using IDCollection = std::vector<MarkID>;
using MarkIDSet = std::set<df::MarkID>;
}  // namespace df
