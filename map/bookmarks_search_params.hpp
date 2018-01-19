#pragma once

#include "drape_frontend/user_marks_global.hpp"

#include "base/assert.hpp"

#include <functional>
#include <string>
#include <vector>

namespace search
{
struct BookmarksSearchParams
{
  using Results = std::vector<df::MarkID>;

  enum class Status
  {
    InProgress,
    Completed,
    Cancelled
  };

  using OnStarted = std::function<void()>;
  using OnResults = std::function<void(Results const & results, Status status)>;

  std::string m_query;
  OnStarted m_onStarted;
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
