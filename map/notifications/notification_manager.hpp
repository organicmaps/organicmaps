#pragma once

#include "map/notifications/notification_queue.hpp"

#include "ugc/api.hpp"

#include "metrics/eye.hpp"

#include <ctime>
#include <memory>
#include <string>

#include <boost/optional.hpp>

namespace notifications
{
using Notification = boost::optional<NotificationCandidate>;
class NotificationManager : public eye::Subscriber
{
public:
  friend class NotificationManagerForTesting;

  class Delegate
  {
  public:
    virtual ~Delegate() = default;
    virtual ugc::Api & GetUGCApi() = 0;
    virtual std::string GetAddress(m2::PointD const & pt) = 0;
  };

  void SetDelegate(std::unique_ptr<Delegate> delegate);

  void Load();
  void TrimExpired();

  Notification GetNotification();
  size_t GetCandidatesCount() const;

  // eye::Subscriber overrides:
  void OnMapObjectEvent(eye::MapObject const & poi) override;

private:
  bool Save();
  void ProcessUgcRateCandidates(eye::MapObject const & poi);
  Candidates::iterator GetUgcRateCandidate();

  std::unique_ptr<Delegate> m_delegate;
  // Notification candidates queue.
  Queue m_queue;
};
}  // namespace notifications
