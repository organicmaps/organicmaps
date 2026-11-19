#include "kinetic_scroller.hpp"
#include "drape_frontend/animation/interpolations.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "visual_params.hpp"

#include <algorithm>

namespace df
{
double constexpr kFlingDuration = 1.5;

// Time window to store move points for better (smooth) _instant_ velocity calculation.
double constexpr kTimeWindowSec = 0.05;
double constexpr kMinDragTimeSec = 0.01;

/// @name Generic pixels per second. Should multiply on visual scale.
/// @{
double constexpr kKineticMaxSpeedStart = 1000.0;
double constexpr kKineticMaxSpeedEnd = 5000.0;
double constexpr kInstantVelocityThreshold = 200.0;
/// @}

double CalculateKineticMaxSpeed(ScreenBase const & screen)
{
  return InterpolateDouble(kKineticMaxSpeedStart, kKineticMaxSpeedEnd, GetNormalizedZoomLevel(screen.GetScale()));
}

void KineticScroller::Init(ScreenBase const & modelView, AnimationClock::time_point now)
{
  ASSERT(!m_isActive, ());
  m_isActive = true;

  m_points.clear();
  m_points.emplace_back(modelView.GetOrg(), now);
}

void KineticScroller::Update(ScreenBase const & modelView, AnimationClock::time_point now)
{
  ASSERT(m_isActive, ());

  if (m_points.size() > 1)
  {
    auto it = std::find_if(m_points.begin(), m_points.end(),
                           [&now](auto const & e) { return GetDurationSeconds(now, e.second) <= kTimeWindowSec; });

    // Keep last point always.
    if (it == m_points.end())
      --it;
    m_points.erase(m_points.begin(), it);
  }

  m_points.emplace_back(modelView.GetOrg(), now);
}

bool KineticScroller::IsActive() const
{
  return m_isActive;
}

// Calculate direction in mercator space, and velocity in pixel space.
// We need the same reaction on different zoom levels, and should calculate velocity on pixel space.
std::pair<m2::PointD, double> KineticScroller::GetDirectionAndVelocity(ScreenBase const & modelView,
                                                                       AnimationClock::time_point now) const
{
  ASSERT(m_isActive, ());
  ASSERT(!m_points.empty(), ());

  if (GetDurationSeconds(now, m_points.back().second) > kTimeWindowSec)
    return {{}, 0};

  auto const delta = modelView.GetOrg() - m_points.front().first;
  double const distance = delta.Length();
  double const elapsed = std::max(kMinDragTimeSec, GetDurationSeconds(now, m_points.front().second));
  double const velocity = distance / (modelView.GetScale() * elapsed);
  if (velocity < kInstantVelocityThreshold * VisualParams::Instance().GetVisualScale())
    return {{}, 0.0};

  return {delta / distance, velocity};
}

void KineticScroller::Cancel()
{
  m_isActive = false;
}

std::optional<MapAnimation> KineticScroller::CreateKineticAnimation(ScreenBase const & modelView,
                                                                    AnimationClock::time_point now)
{
  auto const [direction, releaseSpeed] = GetDirectionAndVelocity(modelView, now);
  Cancel();
  if (releaseSpeed == 0.0)
    return {};

  double const maxSpeed = CalculateKineticMaxSpeed(modelView) * VisualParams::Instance().GetVisualScale();
  double const speed = std::min(releaseSpeed, maxSpeed);
  // Speed falls from the release speed to zero as 1 - t², so its average is two thirds of the release speed.
  auto const & center = modelView.GetOrg();
  auto const target = center + direction * (speed * modelView.GetScale() * kFlingDuration * (2.0 / 3.0));
  auto const & world = GetWorldRect();
  if (target.y < world.minY() || target.y > world.maxY())
    return {};

  ScreenBase end = modelView;
  end.SetOrg(target);
  return MapAnimation(MapAnimation::Type::Fling, modelView, end, now, kFlingDuration);
}
}  // namespace df
