#pragma once

#include "metrics/eye_info.hpp"

#include "base/atomic_shared_ptr.hpp"
#include "base/macros.hpp"

#include <vector>

namespace eye
{
class Subscriber
{
public:
  virtual ~Subscriber(){};

public:
  virtual void OnTipClicked(Tip const & tip){};
  virtual void OnBookingFilterUsed(Time const & time){};
  virtual void OnBookmarksCatalogShown(Time const & time){};
  virtual void OnDiscoveryShown(Time const & time){};
  virtual void OnDiscoveryItemClicked(Discovery::Event event){};
  virtual void OnLayerUsed(Layer const & layer){};
  virtual void OnPlacePageOpened(MapObject const & poi){};
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
    static void PlacePageOpened();
    static void UgcEditorOpened();
    static void UgcSaved();
    static void AddToBookmarkClicked();
    static void RouteCreatedToObject();
  };

  static Eye & Instance();

  InfoType GetInfo() const;
  void Subscribe(Subscriber * subscriber);

private:
  Eye();

  void Save(InfoType const & info);

  // Event processing:
  void RegisterTipClick(Tip::Type type, Tip::Event event);
  void UpdateBookingFilterUsedTime();
  void UpdateBoomarksCatalogShownTime();
  void UpdateDiscoveryShownTime();
  void IncrementDiscoveryItem(Discovery::Event event);
  void RegisterLayerShown(Layer::Type type);
  void RegisterPlacePageOpened();
  void RegisterUgcEditorOpened();
  void RegisterUgcSaved();
  void RegisterAddToBookmarkClicked();
  void RegisterRouteCreatedToObject();

  base::AtomicSharedPtr<Info> m_info;
  std::vector<Subscriber *> m_subscribers;

  DISALLOW_COPY_AND_MOVE(Eye);
};
}  // namespace eye
