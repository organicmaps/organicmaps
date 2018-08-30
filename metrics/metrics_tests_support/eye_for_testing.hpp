#pragma once

#include "metrics/eye_info.hpp"

namespace eye
{
class EyeForTesting
{
public:
  static void ResetEye();
  static void AppendTip(Tip::Type type, Tip::Event event);
  static void SetInfo(Info const & info);
};

class ScopedEyeForTesting
{
public:
  ScopedEyeForTesting() { EyeForTesting::ResetEye(); }
  ~ScopedEyeForTesting() { EyeForTesting::ResetEye(); }
};
}  // namespace eye
