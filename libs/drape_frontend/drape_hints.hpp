#pragma once

namespace df
{
struct Hints
{
  // Zero keeps the default frame pacing, including the route-following limit.
  int m_maxFps = 0;
  bool m_isFirstLaunch = false;
  bool m_isLaunchByDeepLink = false;
  bool m_screenshotMode = false;
  bool m_isPassiveNavigation = false;
  bool m_showPoi = true;
};
}  // namespace df
