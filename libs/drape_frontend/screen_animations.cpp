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

Pose GetPose(ScreenBase const & screen)
{
  return {screen.GetOrg(), screen.GetScale(), screen.GetAngle()};
}

// The cubic bezier easing from (0, 0) to (1, 1) through the x1, y1, x2, y2 control points.
double GetEasing(std::array<double, 4> const & curve, double x)
{
  if (x <= 0.0 || x >= 1.0)
    return x <= 0.0 ? 0.0 : 1.0;

  auto const bezier = [](double t, double p1, double p2)
  { return (((1.0 + 3.0 * (p1 - p2)) * t + 3.0 * (p2 - 2.0 * p1)) * t + 3.0 * p1) * t; };
  // The bezier x grows monotonically with its parameter for control points between 0 and 1.
  double lo = 0.0;
  double hi = 1.0;
  for (int i = 0; i < 32; ++i)
  {
    double const t = (lo + hi) / 2.0;
    (bezier(t, curve[0], curve[2]) < x ? lo : hi) = t;
  }
  return bezier((lo + hi) / 2.0, curve[1], curve[3]);
}

Stage GetFlyStage(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  auto const & rect = startScreen.PixelRectIn3d();
  double const distance = startScreen.GetOrg().Length(endScreen.GetOrg());

  Stage stage;
  stage.m_path = Stage::Path::Fly;
  stage.m_start = GetPose(startScreen);
  stage.m_end = GetPose(endScreen);
  stage.m_flightScale =
      std::max(startScreen.GetScale(), distance / (kFlyViewports * std::min(rect.SizeX(), rect.SizeY())));
  double const zoomLevels = std::fabs(std::log2(endScreen.GetScale() / stage.m_flightScale));
  stage.m_moveDuration = std::min(kMinFlyTimeSec + kFlyTimeSecPerZoomLevel * zoomLevels, kMaxFlyTimeSec);
  return stage;
}

// The scale of a flight at the zoomOut time fraction of its zoom-out.
double GetZoomOutScale(Stage const & stage, double zoomOut)
{
  return stage.m_start.m_scale * std::pow(stage.m_flightScale / stage.m_start.m_scale, zoomOut);
}

Stage GetFollowStage(ScreenBase const & screen, m2::PointD const & userPos, m2::PointD const & endPixelPos,
                     double endScale, double endAngle, bool isAutoZoom, double maxScaleDuration)
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
  stage.m_moveDuration = std::min(move, kMaxAnimationTimeSec);
  if (rotate > 0.0)
    stage.m_rotateDuration = std::min(std::max(rotate, duration), kMaxAnimationTimeSec);
  if (startScale != endScale)
    stage.m_scaleDuration = std::min(std::max(scale, duration), maxScaleDuration);
  return stage;
}
}  // namespace

MapAnimation::MapAnimation(Type type, Stage const & stage, AnimationClock::time_point requestedAt, bool syncArrow)
  : Animation(requestedAt, stage.GetDuration())
  , m_type(type)
  , m_stage(stage)
  , m_syncArrow(syncArrow)
{
  ASSERT(!syncArrow || stage.m_path == Stage::Path::Follow, ());
}

void MapAnimation::Apply(ScreenBase & screen) const
{
  Apply(m_stage, GetElapsed(), screen);
}

void MapAnimation::ApplyTarget(ScreenBase & screen) const
{
  Apply(m_stage, m_stage.GetDuration(), screen);
}

double MapAnimation::GetRemainingScaleTime(AnimationClock::time_point now) const
{
  // A flight zooms for its whole time, the other paths for their own scale time.
  return GetRemainingTime(now, m_stage.m_path == Stage::Path::Fly ? m_stage.m_moveDuration : m_stage.m_scaleDuration);
}

void MapAnimation::SyncArrow(double & moveDuration, double & rotateDuration) const
{
  if (!m_syncArrow)
    return;

  // The arrow moves with the position under m_pixel and turns with the map.
  moveDuration = m_stage.m_moveDuration;
  if (m_stage.m_rotateDuration > 0.0)
    rotateDuration = m_stage.m_rotateDuration;
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
  case Stage::Path::Fly:
  {
    double const arrival = GetEasing(kFlyZoomEasing, (move - kFlyZoomStart) / (1.0 - kFlyZoomStart));
    double const flightScale = stage.m_flightScale;
    double const arrivalScale = InterpolateDouble(flightScale, stage.m_end.m_scale, arrival);
    double const zoomOut = GetTimeFraction(elapsed, kFlyZoomOutTimeSec);
    double const scale = zoomOut < 1.0 ? GetZoomOutScale(stage, zoomOut) : arrivalScale;
    // The pan gains its speed with the zoom-out, so the start position doesn't leave the screen in the first frame.
    double pan = GetEasing(kFlyPanEasing, move);
    if (zoomOut < 1.0)
      pan *= (scale - stage.m_start.m_scale) / (flightScale - stage.m_start.m_scale);
    // The offset to the end position shrinks on the screen with the pan, and the map zooms around the center.
    auto const position = stage.m_end.m_position - (stage.m_end.m_position - stage.m_start.m_position) *
                                                       ((1.0 - pan) * arrivalScale / flightScale);
    screen.SetFromParams(position, InterpolateAngle(stage.m_start.m_angle, stage.m_end.m_angle, pan), scale);
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
  double constexpr kEps = 1e-5;

  ASSERT_GREATER(scale, 0.0, ());
  double const pixelLength = globalDistance / scale;
  if (pixelLength < kEps)
    return 0.0;

  // The time is monotonic: kShortMoveTimeSec is its lower limit, so a longer move never ends sooner than a shorter one.
  double const minSize = std::min(viewportRect.SizeX(), viewportRect.SizeY());
  return std::max(kShortMoveTimeSec, pixelLength / (kMoveViewportsPerSec * minSize));
}

double GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & screen)
{
  return GetMoveDuration(endPosition.Length(startPosition), screen.PixelRectIn3d(), screen.GetScale());
}

double GetScaleDuration(double startScale, double endScale, bool isAutoZoom)
{
  double const ratio = std::max(startScale, endScale) / std::min(startScale, endScale);
  return ratio * (isAutoZoom ? kAutoZoomTimeSecPerScaleRatio : kZoomTimeSecPerScaleRatio);
}

double GetRotateDuration(double startAngle, double endAngle)
{
  startAngle = ang::AngleIn2PI(startAngle);
  endAngle = ang::AngleIn2PI(endAngle);
  return kHalfTurnTimeSec * std::fabs(ang::GetShortestDistance(startAngle, endAngle)) / math::pi;
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
  if (stage.GetDuration() == 0.0)
    return {};
  if (IsAnimationAllowed(stage.GetDuration()))
    return MapAnimation(type, stage, requestedAt);
  return MapAnimation(type, GetFlyStage(startScreen, endScreen), requestedAt);
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
    auto const stage =
        GetFollowStage(startScreen, userPos, endPixelPos, endScale, endAngle, isAutoZoom, maxScaleDuration);
    if (stage.GetDuration() == 0.0)
      return {};
    return MapAnimation(MapAnimation::Type::Camera, stage, requestedAt, true /* syncArrow */);
  }

  return MapAnimation(MapAnimation::Type::Camera, GetFlyStage(startScreen, endScreen), requestedAt);
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
  return MapAnimation(MapAnimation::Type::Zoom, stage, requestedAt);
}
}  // namespace df
