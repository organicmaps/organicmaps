#pragma once

#include "metrics/eye_info.hpp"

#include "base/atomic_shared_ptr.hpp"
#include "base/macros.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace eye
{
class Subscriber
{
public:
  virtual ~Subscriber() = default;

public:
  virtual void OnTipClicked(Tip const & tip) {}
  virtual void OnBookingFilterUsed(Time const & time) {}
  virtual void OnBookmarksCatalogShown(Time const & time) {}
  virtual void OnDiscoveryShown(Time const & time) {}
  virtual void OnDiscoveryItemClicked(Discovery::Event event) {}
  virtual void OnLayerShown(Layer const & layer) {}
  virtual void OnMapObjectEvent(MapObject const & poi) {}
};

// Note This class IS thread-safe.
// All write operations are asynchronous and work on Platform::Thread::File thread.
// Read operations are synchronous and return shared pointer with constant copy of internal
// container.
class Eye
{
public:
  friend class EyeForTesting;
  using InfoType = base::AtomicSharedPtr<Info>::ValueType;

  class Event
  {
  public:
    static void TipClicked(Tip::Type type, Tip::Event event);
    static void BookingFilterUsed();
    static void BoomarksCatalogShown();
    static void DiscoveryShown();
    static void DiscoveryItemClicked(Discovery::Event event);
    static void LayerShown(Layer::Type type);
    static void MapObjectEvent(MapObject const & mapObject, MapObject::Event::Type type,
                               m2::PointD const & userPos);
  };

  static Eye & Instance();

  InfoType GetInfo() const;

  // Subscribe/Unsubscribe must be called from main thread only.
  void Subscribe(Subscriber * subscriber);
  void UnsubscribeAll();

  static std::chrono::hours const & GetMapObjectEventsExpirePeriod();
  void TrimExpired();

private:
  Eye();

  bool Save(InfoType const & info);
  void TrimExpiredMapObjectEvents();

  // Event processing:
  void RegisterTipClick(Tip::Type type, Tip::Event event);
  void UpdateBookingFilterUsedTime();
  void UpdateBoomarksCatalogShownTime();
  void UpdateDiscoveryShownTime();
  void IncrementDiscoveryItem(Discovery::Event event);
  void RegisterLayerShown(Layer::Type type);
  void RegisterMapObjectEvent(MapObject const & mapObject, MapObject::Event::Type type,
                              m2::PointD const & userPos);

  base::AtomicSharedPtr<Info> m_info;
  std::vector<Subscriber *> m_subscribers;

  DISALLOW_COPY_AND_MOVE(Eye);
};
}  // namespace eye
