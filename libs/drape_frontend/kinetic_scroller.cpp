#include "kinetic_scroller.hpp"
#include "drape_frontend/animation/interpolations.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "visual_params.hpp"

#include <algorithm>

namespace df
{
double constexpr kKineticDuration = 1.5;
double constexpr kKineticAcceleration = 0.4;

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

  // A finger that stopped before the release doesn't fling: the points come only with its moves.
  if (GetDurationSeconds(now, m_points.back().second) > kTimeWindowSec)
    return {{}, 0};

  m2::PointD const delta = modelView.GetOrg() - m_points.front().first;
  double const length = delta.Length();
  double const elapsedSec = std::max(kMinDragTimeSec, GetDurationSeconds(now, m_points.front().second));
  double const vs = VisualParams::Instance().GetVisualScale();

  // Most touch filtrations happen here.
  double const velocity = length / modelView.GetScale() / elapsedSec;
  if (velocity < kInstantVelocityThreshold * vs)
    return {{}, 0};

  return {delta / length, std::min(velocity, CalculateKineticMaxSpeed(modelView) * vs)};
}

void KineticScroller::Cancel()
{
  m_isActive = false;
}

std::optional<MapAnimation> KineticScroller::CreateKineticAnimation(ScreenBase const & modelView,
                                                                    AnimationClock::time_point now)
{
  auto const [dir, velocity] = GetDirectionAndVelocity(modelView, now);
  // Cancel current animation in any case.
  Cancel();
  if (velocity < 1E-6)
    return {};

  // The velocity is in pixels, the travel is in mercator.
  m2::PointD const center = modelView.GetOrg();
  double const glbLength = kKineticAcceleration * velocity * modelView.GetScale();
  m2::PointD const target = center + dir * glbLength;
  m2::RectD const & worldR = df::GetWorldRect();
  if (target.y < worldR.minY() || target.y > worldR.maxY())
    return {};

  MapAnimation::Stage stage;
  stage.m_path = MapAnimation::Stage::Path::Fling;
  stage.m_start = {center, modelView.GetScale(), modelView.GetAngle()};
  stage.m_end = {target, modelView.GetScale(), modelView.GetAngle()};
  stage.m_moveDuration = kKineticDuration;
  return MapAnimation(MapAnimation::Type::Fling, {stage}, now);
}
}  // namespace df
