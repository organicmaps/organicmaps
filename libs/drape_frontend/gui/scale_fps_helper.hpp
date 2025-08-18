#pragma once

#include "base/timer.hpp"

#include <cstdint>

namespace gui
{
class ScaleFpsHelper
{
public:
  uint32_t GetFps() const
  {
    if (m_frameTime == 0.0)
      return 0;
    return static_cast<uint32_t>(1.0 / m_frameTime);
  }

  bool IsPaused() const { return m_isPaused; }

  void SetFrameTime(double frameTime, bool isActiveFrame)
  {
    m_isPaused = !isActiveFrame;
    m_framesCounter++;
    m_aggregatedFrameTime += frameTime;

    double constexpr kAggregationPeriod = 0.5;
    if (m_isPaused || m_fpsAggregationTimer.ElapsedSeconds() >= kAggregationPeriod)
    {
      m_frameTime = m_aggregatedFrameTime / m_framesCounter;
      m_aggregatedFrameTime = 0.0;
      m_framesCounter = 0;
      m_fpsAggregationTimer.Reset();
    }
  }

  int GetScale() const { return m_scale; }
  void SetScale(int scale) { m_scale = scale; }

  void SetVisible(bool isVisible) { m_isVisible = isVisible; }
  bool IsVisible() const { return m_isVisible; }

private:
  double m_frameTime = 0.0;
  int m_scale = 1;
  bool m_isVisible = false;

  base::Timer m_fpsAggregationTimer;
  double m_aggregatedFrameTime = 0.0;
  uint32_t m_framesCounter = 1;
  bool m_isPaused = false;
};
}  // namespace gui
