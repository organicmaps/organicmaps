#pragma once

#include "search/bookmarks/types.hpp"

#include <vector>

namespace search
{
namespace bookmarks
{
struct Result
{
  explicit Result(Id id) : m_id(id) {}

  Id m_id = {};
};

using Results = std::vector<Result>;
}  // namespace bookmarks
}  // namespace search
