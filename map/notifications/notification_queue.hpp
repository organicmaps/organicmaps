#pragma once

#include "metrics/eye_info.hpp"

#include <deque>
#include <memory>
#include <string>

namespace notifications
{
using Clock = std::chrono::system_clock;
using Time = Clock::time_point;

struct NotificationCandidate
{
  enum class Type : uint8_t
  {
    UgcAuth = 0,
    UgcReview = 1
  };

  DECLARE_VISITOR(visitor(m_type, "type"), visitor(m_created, "created_time"),
                  visitor(m_used, "used"), visitor(m_mapObject, "object"));

  Type m_type;
  Time m_created;
  Time m_used;
  std::shared_ptr<eye::MapObject> m_mapObject;
};

using Candidates = std::deque<NotificationCandidate>;

enum class Version : int8_t
{
  Unknown = -1,
  V0 = 0,
  Latest = V0
};

struct QueueV0
{
  static Version GetVersion() { return Version::V0; }

  DECLARE_VISITOR(visitor(m_candidates, "queue"),
                  visitor(m_lastNotificationProvidedTime, "last_notification"));

  Time m_lastNotificationProvidedTime;
  Candidates m_candidates;
};

using Queue = QueueV0;

inline std::string DebugPrint(NotificationCandidate::Type type)
{
  switch (type)
  {
  case NotificationCandidate::Type::UgcAuth: return "UgcAuth";
  case NotificationCandidate::Type::UgcReview: return "UgcReview";
  }
}
}  // namespace notifications
