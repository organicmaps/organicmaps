#pragma once

#include "kml/type_utils.hpp"

#include "base/assert.hpp"

#include <functional>
#include <string>
#include <vector>

namespace search
{
struct BookmarksSearchParams
{
  using Results = std::vector<kml::MarkId>;

  enum class Status
  {
    InProgress,
    Completed,
    Cancelled
  };

  std::string m_query;
  kml::MarkGroupId m_groupId = kml::kInvalidMarkGroupId;

  using OnResults = std::function<void(Results results, Status status)>;
  OnResults m_onResults;
};

inline std::string DebugPrint(BookmarksSearchParams::Status status)
{
  using Status = BookmarksSearchParams::Status;
  switch (status)
  {
  case Status::InProgress: return "InProgress";
  case Status::Completed: return "Completed";
  case Status::Cancelled: return "Cancelled";
  }
  ASSERT(false, ("Unknown status"));
  return "Unknown";
}
}  // namespace search
