#pragma once

#include "metrics/eye_info.hpp"

#include <deque>
#include <memory>
#include <string>

namespace notifications
{
struct Notification
{
  enum class Type : uint8_t
  {
    UgcAuth,
    UgcReview
  };

  DECLARE_VISITOR(visitor(m_type, "type"), visitor(m_mapObject, "object"));

  Type m_type;
  std::unique_ptr<eye::MapObject> m_mapObject;
};

using Candidates = std::deque<Notification>;

enum class Version : int8_t
{
  Unknown = -1,
  V0 = 0,
  Latest = V0
};

struct QueueV0
{
  static Version GetVersion() { return Version::V0; }

  DECLARE_VISITOR(visitor(m_candidates, "queue"))

  Candidates m_candidates;
};

using Queue = QueueV0;

inline std::string DebugPrint(Notification::Type type)
{
  switch (type)
  {
  case Notification::Type::UgcAuth: return "UgcAuth";
  case Notification::Type::UgcReview: return "UgcReview";
  }
}
}  // namespace notifications
