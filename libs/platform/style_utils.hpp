#pragma once

#include <cstdint>

namespace style_utils
{
enum class NightMode : uint8_t
{
  Off = 0,
  On = 1,
  System = 2,
};

NightMode GetNightModeSetting();
void SetNightModeSetting(NightMode mode);

}  // namespace style_utils
