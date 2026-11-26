#include "drape_frontend/screen_animations.hpp"

#include "drape_frontend/animation/interpolations.hpp"

#include "base/math.hpp"

namespace df
{
MapAnimation::MapAnimation(Type type, ScreenBase const & start, ScreenBase const & end,
                           AnimationClock::time_point requestedAt, double duration,
                           std::optional<m2::PointD> pixelAnchor)
  : Animation(requestedAt, duration, type == Type::Fling ? kMaxFlingDurationSec : kMaxAnimationTimeSec)
  , m_type(type)
  , m_pixelAnchor(pixelAnchor)
  , m_start(GetPose(start, pixelAnchor))
  , m_end(GetPose(end, pixelAnchor))
{}

MapAnimation::Pose MapAnimation::GetPose(ScreenBase const & screen, std::optional<m2::PointD> const & pixelAnchor)
{
  auto const position = pixelAnchor ? screen.PtoG(screen.P3dtoP(*pixelAnchor)) : screen.GetOrg();
  return {position, screen.GetScale(), screen.GetAngle()};
}

void MapAnimation::Apply(ScreenBase & screen, double progress) const
{
  auto const position = InterpolatePoint(m_start.m_position, m_end.m_position, progress);
  if (m_type == Type::Fling)
  {
    screen.SetOrg(position);
    return;
  }

  screen.SetFromParams(position, InterpolateAngle(m_start.m_angle, m_end.m_angle, progress),
                       InterpolateScale(m_start.m_scale, m_end.m_scale, progress));
  if (m_pixelAnchor)
    screen.MatchGandP3d(position, *m_pixelAnchor);
}

bool IsWithinAnimationDistance(ScreenBase const & screen, m2::PointD const & from, m2::PointD const & to)
{
  double constexpr kMaxViewports = 2.0;
  auto const & viewport = screen.PixelRect();
  auto const delta = screen.GtoP(to) - screen.GtoP(from);
  m2::PointD const viewports(delta.x / viewport.SizeX(), delta.y / viewport.SizeY());
  return viewports.SquaredLength() <= math::Pow2(kMaxViewports);
}
}  // namespace df
