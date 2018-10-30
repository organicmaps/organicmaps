#pragma once

#include "metrics/eye_info.hpp"

#include "platform/platform.hpp"

namespace eye
{
class EyeForTesting
{
public:
  static void ResetEye();
  static void SetInfo(Info const & info);
  static void AppendTip(Tip::Type type, Tip::Event event);
  static void UpdateBookingFilterUsedTime();
  static void UpdateBoomarksCatalogShownTime();
  static void UpdateDiscoveryShownTime();
  static void IncrementDiscoveryItem(Discovery::Event event);
  static void AppendLayer(Layer::Type type);
  static void TrimExpiredMapObjectEvents();
  static void RegisterMapObjectEvent(MapObject const & mapObject, MapObject::Event::Type type,
                                     m2::PointD const & userPos);
};

class ScopedEyeForTesting
{
public:
  ScopedEyeForTesting() { EyeForTesting::ResetEye(); }
  ~ScopedEyeForTesting() { EyeForTesting::ResetEye(); }

private:
  Platform::ThreadRunner m_runner;
};
}  // namespace eye
