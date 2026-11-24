#include "kinetic_scroller.hpp"
#include "drape_frontend/animation/interpolations.hpp"
#include "drape_frontend/screen_operations.hpp"
#include "visual_params.hpp"

#include <algorithm>
#include <cmath>

namespace df
{
namespace
{
// Time window to store move points for better (smooth) _instant_ velocity calculation.
double constexpr kTimeWindowSec = 0.05;
double constexpr kMinDragTimeSec = 0.01;

// Minimum fling speed in pixels per second at visual scale 1.
double constexpr kInstantVelocityThreshold = 200.0;

// Cubic ease-out starts at three times the average speed.
double constexpr kFlingInitialToAverageSpeedRatio = 3.0;

}  // namespace

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

  auto const viewport = ViewportFootprint(modelView, modelView.PixelRectIn3d()).m_bounds;
  // Use the shorter viewport dimension to set the duration for a given release speed in any direction.
  double const speedForFullDuration = std::min(viewport.SizeX(), viewport.SizeY()) * kMaxFlingViewports *
                                      kFlingInitialToAverageSpeedRatio / kMaxFlingDurationSec;
  double const duration =
      InterpolateDouble(kMinFlingDurationSec, kMaxFlingDurationSec, std::min(releaseSpeed / speedForFullDuration, 1.0));
  auto const screenDirection = m2::Rotate(direction, -modelView.GetAngle());
  double const viewportsPerPixel =
      std::max(std::abs(screenDirection.x) / viewport.SizeX(), std::abs(screenDirection.y) / viewport.SizeY());
  double const distancePixels =
      std::min(releaseSpeed * duration / kFlingInitialToAverageSpeedRatio, kMaxFlingViewports / viewportsPerPixel);
  auto const delta = direction * (distancePixels * modelView.GetScale());
  double fraction = 1.0;
  if (delta.y != 0.0)
  {
    auto const & clip = modelView.ClipRect();
    auto const & world = GetWorldRect();
    double const room = delta.y > 0.0 ? world.maxY() - clip.maxY() : clip.minY() - world.minY();
    if (room <= 0.0)
      return {};
    fraction = std::min(1.0, room / std::abs(delta.y));
  }

  // Shorten both travel and time at a pole to keep the release velocity and heading.
  ScreenBase end = modelView;
  end.SetOrg(modelView.GetOrg() + delta * fraction);
  return MapAnimation(MapAnimation::Type::Fling, modelView, end, now, duration * fraction);
}
}  // namespace df
