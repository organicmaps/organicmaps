#include "drape_frontend/screen_animations.hpp"

#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/angles.hpp"

#include "base/math.hpp"

#include <cmath>

namespace df
{
namespace
{
using Pose = MapAnimation::Pose;
using Stage = MapAnimation::Stage;

double constexpr kKineticFadeoff = 4.0;
double constexpr kPrettyZoomTimeSec = kMaxAnimationTimeSec * 0.5;

Pose GetPose(ScreenBase const & screen)
{
  return {screen.GetOrg(), screen.GetScale(), screen.GetAngle()};
}

double GetDuration(MapAnimation::Stages const & stages)
{
  double duration = 0.0;
  for (auto const & stage : stages)
    duration += stage.GetDuration();
  return duration;
}

Stage GetZoomStage(Pose const & start, double endScale)
{
  Stage stage;
  stage.m_start = start;
  stage.m_end = {start.m_position, endScale, start.m_angle};
  if (start.m_scale != endScale)
    stage.m_scaleDuration =
        std::min(GetScaleDuration(start.m_scale, endScale, false /* isAutoZoom */), kPrettyZoomTimeSec);
  return stage;
}

Stage GetFollowStage(ScreenBase const & screen, m2::PointD const & userPos, m2::PointD const & endPixelPos,
                     double endScale, double endAngle, bool isAutoZoom, double maxDuration, double maxScaleDuration)
{
  Stage stage;
  stage.m_path = Stage::Path::Follow;
  stage.m_start = {screen.PtoG(screen.P3dtoP(endPixelPos)), screen.GetScale(), screen.GetAngle()};
  stage.m_end = {userPos, endScale, endAngle};
  stage.m_pixel = endPixelPos;

  double const startScale = screen.GetScale();
  double const averageScale = isAutoZoom ? startScale : (startScale + endScale) / 2.0;
  double const move = GetMoveDuration(stage.m_start.m_position.Length(userPos), screen.PixelRectIn3d(), averageScale);
  double const scale = GetScaleDuration(startScale, endScale, isAutoZoom);
  double const rotate = GetRotateDuration(screen.GetAngle(), endAngle);
  // The angle and the scale change until the end of the follow. An auto-zoom may take longer.
  double const duration = std::max({move, rotate, isAutoZoom ? 0.0 : scale});
  stage.m_moveDuration = std::min(move, maxDuration);
  if (rotate > 0.0)
    stage.m_rotateDuration = std::min(std::max(rotate, duration), maxDuration);
  if (startScale != endScale)
    stage.m_scaleDuration = std::min({std::max(scale, duration), maxDuration, maxScaleDuration});
  return stage;
}
}  // namespace

MapAnimation::MapAnimation(Type type, Stages && stages, AnimationClock::time_point requestedAt, bool syncArrow)
  : Animation(requestedAt, GetDuration(stages))
  , m_type(type)
  , m_stages(std::move(stages))
  , m_syncArrow(syncArrow)
{
  ASSERT(!syncArrow || (m_stages.size() == 1 && m_stages.front().m_path == Stage::Path::Follow), ());
}

void MapAnimation::Apply(ScreenBase & screen) const
{
  double elapsed = GetElapsed();
  size_t i = 0;
  for (; i + 1 < m_stages.size() && elapsed >= m_stages[i].GetDuration(); ++i)
    elapsed -= m_stages[i].GetDuration();
  Apply(m_stages[i], elapsed, screen);
}

void MapAnimation::ApplyTarget(ScreenBase & screen) const
{
  auto const & stage = m_stages.back();
  Apply(stage, stage.GetDuration(), screen);
}

double MapAnimation::GetRemainingScaleTime(AnimationClock::time_point now) const
{
  // A single stage zooms for its own time, a distant move zooms in at its end.
  return m_stages.size() == 1 ? GetRemainingTime(now, m_stages.front().m_scaleDuration) : GetRemainingTime(now);
}

void MapAnimation::SyncArrow(double & moveDuration, double & rotateDuration) const
{
  if (!m_syncArrow)
    return;

  // The arrow moves with the position under m_pixel and turns with the map.
  auto const & stage = m_stages.front();
  moveDuration = stage.m_moveDuration;
  if (stage.m_rotateDuration > 0.0)
    rotateDuration = stage.m_rotateDuration;
}

// static
void MapAnimation::Apply(Stage const & stage, double elapsed, ScreenBase & screen)
{
  double const move = GetTimeFraction(elapsed, stage.m_moveDuration);
  double const scale =
      InterpolateDouble(stage.m_start.m_scale, stage.m_end.m_scale, GetTimeFraction(elapsed, stage.m_scaleDuration));
  double const angle =
      InterpolateAngle(stage.m_start.m_angle, stage.m_end.m_angle, GetTimeFraction(elapsed, stage.m_rotateDuration));
  switch (stage.m_path)
  {
  case Stage::Path::Line:
    screen.SetFromParams(InterpolatePoint(stage.m_start.m_position, stage.m_end.m_position, move), angle, scale);
    break;
  case Stage::Path::Fling:
  {
    // The fling slows down exponentially and stops at the end position.
    double const glide = (1.0 - std::exp(-kKineticFadeoff * move)) / (1.0 - std::exp(-kKineticFadeoff));
    screen.SetFromParams(InterpolatePoint(stage.m_start.m_position, stage.m_end.m_position, glide), angle, scale);
    break;
  }
  case Stage::Path::Follow:
  {
    auto const position = InterpolatePoint(stage.m_start.m_position, stage.m_end.m_position, move);
    screen.SetFromParams(position, angle, scale);
    screen.MatchGandP3d(position, stage.m_pixel);
    break;
  }
  }
}

bool IsAnimationAllowed(double duration)
{
  return duration > 0.0 && duration <= kMaxAnimationTimeSec;
}

double GetMoveDuration(double globalDistance, m2::RectD const & viewportRect, double scale)
{
  double constexpr kMinMoveDuration = 0.2;
  double constexpr kMaxSpeedScalar = 7.0;
  double constexpr kEps = 1e-5;

  ASSERT_GREATER(scale, 0.0, ());
  double const pixelLength = globalDistance / scale;
  if (pixelLength < kEps)
    return 0.0;

  // The time is monotonic: kMinMoveDuration is its lower limit, so a longer move never ends sooner than a shorter one.
  double const minSize = std::min(viewportRect.SizeX(), viewportRect.SizeY());
  return std::max(kMinMoveDuration, pixelLength / (kMaxSpeedScalar * minSize));
}

double GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & screen)
{
  return GetMoveDuration(endPosition.Length(startPosition), screen.PixelRectIn3d(), screen.GetScale());
}

double GetScaleDuration(double startScale, double endScale, bool isAutoZoom)
{
  // Resize 2.0 times should be done for 1.2 seconds in autozoom or for 0.2 seconds in usual case.
  double const kPixelSpeed = isAutoZoom ? (2.0 / 1.2) : (2.0 / 0.2);

  if (startScale > endScale)
    std::swap(startScale, endScale);

  return endScale / startScale / kPixelSpeed;
}

double GetRotateDuration(double startAngle, double endAngle)
{
  double constexpr kRotateDurationScalar = 0.75;
  startAngle = ang::AngleIn2PI(startAngle);
  endAngle = ang::AngleIn2PI(endAngle);
  return kRotateDurationScalar * std::fabs(ang::GetShortestDistance(startAngle, endAngle)) / math::pi;
}

std::optional<MapAnimation> GetScreenAnimation(MapAnimation::Type type, ScreenBase const & startScreen,
                                               ScreenBase const & endScreen, AnimationClock::time_point requestedAt,
                                               double maxScaleDuration)
{
  double const startScale = startScreen.GetScale();
  double const endScale = endScreen.GetScale();

  Stage stage;
  stage.m_start = GetPose(startScreen);
  stage.m_end = GetPose(endScreen);
  stage.m_moveDuration = GetMoveDuration(startScreen.GetOrg().Length(endScreen.GetOrg()), startScreen.PixelRectIn3d(),
                                         (startScale + endScale) / 2.0);
  if (startScale != endScale)
    stage.m_scaleDuration = std::min(GetScaleDuration(startScale, endScale, false /* isAutoZoom */), maxScaleDuration);
  stage.m_rotateDuration = GetRotateDuration(startScreen.GetAngle(), endScreen.GetAngle());
  if (IsAnimationAllowed(stage.GetDuration()))
    return MapAnimation(type, {stage}, requestedAt);

  double const moveDuration = GetMoveDuration(startScreen.GetOrg(), endScreen.GetOrg(), startScreen);
  if (moveDuration <= kMaxAnimationTimeSec)
    return {};

  // Zoom out, move and zoom in without rotation.
  double const zoomOutScale = startScale * moveDuration / kMaxAnimationTimeSec * 2.0;
  Stage const zoomOut = GetZoomStage(stage.m_start, zoomOutScale);
  Stage move;
  move.m_start = zoomOut.m_end;
  move.m_end = {endScreen.GetOrg(), zoomOutScale, zoomOut.m_end.m_angle};
  move.m_moveDuration = std::min(moveDuration, kMaxAnimationTimeSec);
  return MapAnimation(type, {zoomOut, move, GetZoomStage(move.m_end, endScale)}, requestedAt);
}

std::optional<MapAnimation> GetFollowAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen,
                                               m2::PointD const & userPos, m2::PointD const & endPixelPos,
                                               bool isAutoZoom, AnimationClock::time_point requestedAt,
                                               double maxScaleDuration)
{
  double const endScale = endScreen.GetScale();
  double const endAngle = endScreen.GetAngle();
  auto const & viewportRect = startScreen.PixelRectIn3d();
  double const moveDuration = GetMoveDuration(startScreen.GetOrg().Length(endScreen.GetOrg()), viewportRect,
                                              (startScreen.GetScale() + endScale) / 2.0);
  if (moveDuration <= kMaxAnimationTimeSec)
  {
    auto const stage = GetFollowStage(startScreen, userPos, endPixelPos, endScale, endAngle, isAutoZoom,
                                      kMaxAnimationTimeSec, maxScaleDuration);
    if (stage.GetDuration() == 0.0)
      return {};
    return MapAnimation(MapAnimation::Type::Camera, {stage}, requestedAt, true /* syncArrow */);
  }

  // Zoom out and move to the user position in the viewport center, then follow it.
  ScreenBase screen = startScreen;
  screen.SetAngle(endAngle);
  screen.MatchGandP3d(userPos, viewportRect.Center());
  double const prettyMoveDuration = GetMoveDuration(startScreen.GetOrg(), screen.GetOrg(), startScreen);

  MapAnimation::Stages stages;
  screen = startScreen;
  if (prettyMoveDuration > 0.0)
  {
    double const zoomOutScale = startScreen.GetScale() * prettyMoveDuration / kMaxAnimationTimeSec * 2.0;
    stages.push_back(GetZoomStage(GetPose(startScreen), zoomOutScale));
    screen.SetScale(zoomOutScale);
    screen.MatchGandP3d(userPos, viewportRect.Center());

    Stage move;
    move.m_start = stages.back().m_end;
    move.m_end = GetPose(screen);
    move.m_moveDuration =
        std::min(GetMoveDuration(startScreen.GetOrg().Length(screen.GetOrg()), viewportRect, zoomOutScale),
                 kMaxAnimationTimeSec);
    stages.push_back(move);
  }
  stages.push_back(GetFollowStage(screen, userPos, endPixelPos, endScale, endAngle, false /* isAutoZoom */,
                                  kPrettyZoomTimeSec, kPrettyZoomTimeSec));
  return MapAnimation(MapAnimation::Type::Camera, std::move(stages), requestedAt);
}

MapAnimation GetScaleAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen,
                               m2::PointD const & pxScaleCenter, AnimationClock::time_point requestedAt)
{
  // The position under pxScaleCenter also moves when the end screen is kept within the world.
  Stage stage;
  stage.m_path = Stage::Path::Follow;
  stage.m_start = {startScreen.PtoG(startScreen.P3dtoP(pxScaleCenter)), startScreen.GetScale(), startScreen.GetAngle()};
  stage.m_end = {endScreen.PtoG(endScreen.P3dtoP(pxScaleCenter)), endScreen.GetScale(), startScreen.GetAngle()};
  stage.m_pixel = pxScaleCenter;
  stage.m_scaleDuration = std::min(
      GetScaleDuration(startScreen.GetScale(), endScreen.GetScale(), false /* isAutoZoom */), kMaxAnimationTimeSec);
  stage.m_moveDuration = stage.m_scaleDuration;
  return MapAnimation(MapAnimation::Type::Zoom, {stage}, requestedAt);
}
}  // namespace df
