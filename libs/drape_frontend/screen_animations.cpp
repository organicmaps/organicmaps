#include "drape_frontend/screen_animations.hpp"

#include "drape_frontend/animation/interpolations.hpp"

#include "base/math.hpp"

namespace df
{
MapAnimation::MapAnimation(Type type, ScreenBase const & start, ScreenBase const & end,
                           AnimationClock::time_point requestedAt, double duration,
                           std::optional<m2::PointD> pixelAnchor)
  : Animation(requestedAt, duration)
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

void MapAnimation::Apply(ScreenBase & screen) const
{
  double const t = GetTimeFraction();
  // The fling's speed falls as 1 - t². The position curve is its normalized integral.
  Apply(screen, m_type == Type::Fling ? t * (3.0 - t * t) * 0.5 : GetProgress());
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

bool IsLocalCameraMove(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  double constexpr kMaxZoomRatio = 4.0;
  double const zoomRatio = endScreen.GetScale() / startScreen.GetScale();
  if (zoomRatio < 1.0 / kMaxZoomRatio || zoomRatio > kMaxZoomRatio)
    return false;

  auto const & viewport = startScreen.PixelRect();
  m2::PointD const delta = startScreen.GtoP(endScreen.GetOrg()) - viewport.Center();
  return math::Pow2(delta.x / viewport.SizeX()) + math::Pow2(delta.y / viewport.SizeY()) <= 1.0;
}
}  // namespace df
