#include "platform/style_utils.hpp"

#include "platform/settings.hpp"

namespace style_utils
{
namespace
{
bool IsValidNightModeValue(int value)
{
  return value >= static_cast<int>(NightMode::Off) && value <= static_cast<int>(NightMode::System);
}
}  // namespace

NightMode GetNightModeSetting()
{
  int value = static_cast<int>(NightMode::Off);
  if (settings::Get(settings::kNightMode, value) && IsValidNightModeValue(value))
    return static_cast<NightMode>(value);
  return NightMode::Off;
}

void SetNightModeSetting(NightMode mode)
{
  settings::Set(settings::kNightMode, static_cast<int>(mode));
}

}  // namespace style_utils
