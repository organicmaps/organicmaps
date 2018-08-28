#pragma once

#include "eye/eye_info.hpp"

namespace eye
{
class EyeForTesting
{
public:
  static void ResetEye();
  static void AppendTip(Tips::Type type, Tips::Event event);
  static void SetInfo(Info const & info);
};

class ScopedEyeForTesting
{
public:
  ScopedEyeForTesting() { EyeForTesting::ResetEye(); }
  ~ScopedEyeForTesting() { EyeForTesting::ResetEye(); }
};
}  // namespace eye
