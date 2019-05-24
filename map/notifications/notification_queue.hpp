#pragma once

#include "metrics/eye_info.hpp"

#include <deque>
#include <memory>
#include <string>

namespace notifications
{
using Clock = std::chrono::system_clock;
using Time = Clock::time_point;

class NotificationCandidate
{
public:
  friend class NotificationManagerForTesting;

  enum class Type : uint8_t
  {
    UgcAuth = 0,
    UgcReview = 1
  };

  DECLARE_VISITOR(visitor(m_type, "type"), visitor(m_created, "created_time"),
                  visitor(m_used, "used"), visitor(m_mapObject, "object"),
                  visitor(m_address, std::string(""), "address"));

  NotificationCandidate() = default;
  NotificationCandidate(Type type);
  // Constructs candidate with type Type::UgcReview.
  NotificationCandidate(eye::MapObject const & poi, std::string const & address);

  Type GetType() const;
  Time GetCreatedTime() const;
  Time GetLastUsedTime() const;
  bool IsUsed() const;
  void MarkAsUsed();

  // Methods for Type::UgcReview type.
  // It is possible to use inheritance, but our serialization/deserialization
  // mechanism is not support it.
  bool IsSameMapObject(eye::MapObject const & rhs) const;
  std::string const & GetBestFeatureType() const;
  m2::PointD const & GetPos() const;
  std::string const & GetDefaultName() const;
  std::string const & GetReadableName() const;
  std::string const & GetAddress() const;

  void SetBestFeatureType(std::string const & bestFeatureType);
  void SetPos(m2::PointD const & pt);
  void SetDefaultName(std::string const & name);
  void SetReadableName(std::string const & name);
  void SetAddress(std::string const & address);

private:
  Type m_type;
  Time m_created;
  Time m_used;
  std::shared_ptr<eye::MapObject> m_mapObject;
  std::string m_address;
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
                  visitor(m_lastNotificationProvidedTime, "last_notification"))

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
  UNREACHABLE();
}
}  // namespace notifications
