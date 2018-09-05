#pragma once

#include "metrics/eye_info.hpp"

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
};

class ScopedEyeForTesting
{
public:
  ScopedEyeForTesting() { EyeForTesting::ResetEye(); }
  ~ScopedEyeForTesting() { EyeForTesting::ResetEye(); }
};
}  // namespace eye
