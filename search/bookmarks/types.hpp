#pragma once

#include "search/bookmarks/data.hpp"

#include <cstdint>
#include <limits>

namespace search
{
namespace bookmarks
{
// todo(@m) s/Id/DocId/g ?
using Id = uint64_t;
using GroupId = uint64_t;
using Doc = Data;

GroupId constexpr kInvalidGroupId = std::numeric_limits<GroupId>::max();
}  // namespace bookmarks
}  // namespace search
