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

extern GroupId const kInvalidGroupId;
}  // namespace bookmarks
}  // namespace search
