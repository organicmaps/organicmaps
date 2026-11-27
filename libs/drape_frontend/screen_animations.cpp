#include "drape_frontend/screen_animations.hpp"

#include "drape_frontend/animation/interpolations.hpp"

#include "geometry/angles.hpp"

#include "base/math.hpp"

#include <cmath>
#include <utility>

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

// The angle of the endAngle direction that is the shortest turn away from startAngle.
double GetNearestAngle(double startAngle, double endAngle)
{
  return startAngle + ang::GetShortestDistance(startAngle, endAngle);
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

// The scale that shows the distance between the screen centers as kFlyViewports shorter sides, at least the start one.
double GetFlightScale(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  auto const & rect = startScreen.PixelRectIn3d();
  double const distance = startScreen.GetOrg().Length(endScreen.GetOrg());
  return std::max(startScreen.GetScale(), distance / (kFlyViewports * std::min(rect.SizeX(), rect.SizeY())));
}

bool IsFlightNeeded(double flightScale, ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  return flightScale > std::max(startScreen.GetScale(), endScreen.GetScale());
}

bool IsFlightNeeded(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  return IsFlightNeeded(GetFlightScale(startScreen, endScreen), startScreen, endScreen);
}

// A move, or a flight when neither screen shows the distance within kFlyViewports.
Stage GetMoveStage(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  double const flightScale = GetFlightScale(startScreen, endScreen);
  bool const isFlight = IsFlightNeeded(flightScale, startScreen, endScreen);
  Stage stage;
  stage.m_start = GetPose(startScreen);
  stage.m_end = GetPose(endScreen);
  stage.m_end.m_angle = GetNearestAngle(stage.m_start.m_angle, endScreen.GetAngle());
  stage.m_flightScale = isFlight ? flightScale : startScreen.GetScale();
  double const zoomLevels = std::fabs(std::log2(endScreen.GetScale() / stage.m_flightScale));
  if (isFlight)
  {
    stage.m_path = Stage::Path::Fly;
    stage.m_moveDuration = std::min(kMinFlyTimeSec + kFlyTimeSecPerZoomLevel * zoomLevels, kMaxFlyTimeSec);
  }
  else
  {
    stage.m_moveDuration = std::min(std::max({GetMoveDuration(startScreen.GetOrg(), endScreen.GetOrg(), startScreen),
                                              kMoveTimeSecPerZoomLevel * zoomLevels,
                                              GetRotateDuration(startScreen.GetAngle(), endScreen.GetAngle())}),
                                    kMaxMoveTimeSec);
  }
  return stage;
}

// The scale of a flight at the zoomOut time fraction of its zoom-out.
double GetZoomOutScale(Stage const & stage, double zoomOut)
{
  return stage.m_start.m_scale * std::pow(stage.m_flightScale / stage.m_start.m_scale, zoomOut);
}

// The pan to the end position and the way from the flight scale to the end scale at the move and zoom-out time
// fractions.
std::pair<double, double> GetPanAndArrival(Stage const & stage, double move, double zoomOut)
{
  if (stage.m_path != Stage::Path::Fly)
    return {GetEasing(kMovePanEasing, move), GetEasing(kMoveZoomEasing, move)};

  // The pan gains its speed with the zoom-out, so the start position doesn't leave the screen in the first frame.
  double pan = GetEasing(kFlyPanEasing, move);
  if (zoomOut < 1.0)
    pan *= (GetZoomOutScale(stage, zoomOut) - stage.m_start.m_scale) / (stage.m_flightScale - stage.m_start.m_scale);
  return {pan, GetEasing(kFlyZoomEasing, (move - kFlyZoomStart) / (1.0 - kFlyZoomStart))};
}

// The part of the start offset from the end position that is left at the pan. During a zoom-in it shrinks with the
// scale too, so the end position moves along a line on the screen. During a zoom-out it stays on the map, so the end
// position doesn't run away with the zoomed-out screen.
double GetOffsetShare(double pan, double arrivalScale, double flightScale)
{
  return (1.0 - pan) * std::min(1.0, arrivalScale / flightScale);
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

void MapAnimation::SyncArrow(double & moveDuration, double & rotateDuration) const
{
  if (!m_syncArrow)
    return;

  // The arrow moves with the position under m_pixel and turns with the map.
  moveDuration = m_stage.m_moveDuration;
  if (m_stage.m_rotateDuration > 0.0)
    rotateDuration = m_stage.m_rotateDuration;
}

double MapAnimation::GetRemainingScaleTime(AnimationClock::time_point now) const
{
  // A follow zooms for its own time, the other paths for the whole move.
  return GetRemainingTime(now,
                          m_stage.m_path == Stage::Path::Follow ? m_stage.m_scaleDuration : m_stage.m_moveDuration);
}

bool MapAnimation::Retarget(ScreenBase const & endScreen, AnimationClock::time_point now)
{
  auto & stage = m_stage;
  if (stage.m_path != Stage::Path::Move && stage.m_path != Stage::Path::Fly)
    return false;

  // A new animation from the current screen takes a target that the old one would need a flight to, or that the rest
  // of this animation is too short to reach without a jump.
  ScreenBase oldEnd = endScreen;
  ApplyTarget(oldEnd);
  if (IsFlightNeeded(oldEnd, endScreen) || GetRemainingTime(now) < GetMoveStage(oldEnd, endScreen).m_moveDuration)
    return false;

  auto const [pan, arrival] =
      GetPanAndArrival(stage, GetTimeFraction(stage.m_moveDuration), GetTimeFraction(kFlyZoomOutTimeSec));
  if (pan == 1.0 || arrival == 1.0)
    return false;

  // Keep the current screen and change the flight scale, the start position and the start angle so that the rest of
  // the flight ends at the new target.
  double const arrivalScale = InterpolateDouble(stage.m_flightScale, stage.m_end.m_scale, arrival);
  double const flightScale = (arrivalScale - endScreen.GetScale() * arrival) / (1.0 - arrival);
  if (flightScale <= 0.0)
    return false;

  auto const position = stage.m_end.m_position - (stage.m_end.m_position - stage.m_start.m_position) *
                                                     GetOffsetShare(pan, arrivalScale, stage.m_flightScale);
  double const angle = InterpolateDouble(stage.m_start.m_angle, stage.m_end.m_angle, pan);
  stage.m_end = {endScreen.GetOrg(), endScreen.GetScale(), GetNearestAngle(angle, endScreen.GetAngle())};
  stage.m_flightScale = flightScale;
  stage.m_start.m_position =
      stage.m_end.m_position - (stage.m_end.m_position - position) / GetOffsetShare(pan, arrivalScale, flightScale);
  stage.m_start.m_angle = (angle - stage.m_end.m_angle * pan) / (1.0 - pan);
  return true;
}

// static
void MapAnimation::Apply(Stage const & stage, double elapsed, ScreenBase & screen)
{
  double const move = GetTimeFraction(elapsed, stage.m_moveDuration);
  switch (stage.m_path)
  {
  case Stage::Path::Fling:
  {
    // The fling slows down exponentially and stops at the end position.
    double const glide = (1.0 - std::exp(-kKineticFadeoff * move)) / (1.0 - std::exp(-kKineticFadeoff));
    screen.SetFromParams(InterpolatePoint(stage.m_start.m_position, stage.m_end.m_position, glide), stage.m_end.m_angle,
                         stage.m_end.m_scale);
    break;
  }
  case Stage::Path::Follow:
  {
    auto const position = InterpolatePoint(stage.m_start.m_position, stage.m_end.m_position, move);
    double const scale =
        InterpolateDouble(stage.m_start.m_scale, stage.m_end.m_scale, GetTimeFraction(elapsed, stage.m_scaleDuration));
    double const angle =
        InterpolateAngle(stage.m_start.m_angle, stage.m_end.m_angle, GetTimeFraction(elapsed, stage.m_rotateDuration));
    screen.SetFromParams(position, angle, scale);
    screen.MatchGandP3d(position, stage.m_pixel);
    break;
  }
  case Stage::Path::Move:
  case Stage::Path::Fly:
  {
    double const zoomOut = stage.m_path == Stage::Path::Fly ? GetTimeFraction(elapsed, kFlyZoomOutTimeSec) : 1.0;
    auto const [pan, arrival] = GetPanAndArrival(stage, move, zoomOut);
    double const flightScale = stage.m_flightScale;
    double const arrivalScale = InterpolateDouble(flightScale, stage.m_end.m_scale, arrival);
    double const scale = zoomOut < 1.0 ? GetZoomOutScale(stage, zoomOut) : arrivalScale;
    // The offset to the end position shrinks on the screen with the pan, and the map zooms around the center.
    auto const position = stage.m_end.m_position - (stage.m_end.m_position - stage.m_start.m_position) *
                                                       GetOffsetShare(pan, arrivalScale, flightScale);
    screen.SetFromParams(position, InterpolateDouble(stage.m_start.m_angle, stage.m_end.m_angle, pan), scale);
    break;
  }
  }
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
  return kHalfTurnTimeSec * std::fabs(ang::GetShortestDistance(startAngle, endAngle)) / math::pi;
}

std::optional<MapAnimation> GetScreenAnimation(MapAnimation::Type type, ScreenBase const & startScreen,
                                               ScreenBase const & endScreen, AnimationClock::time_point requestedAt)
{
  auto const stage = GetMoveStage(startScreen, endScreen);
  if (stage.GetDuration() == 0.0)
    return {};
  return MapAnimation(type, stage, requestedAt);
}

std::optional<MapAnimation> GetFollowAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen,
                                               m2::PointD const & userPos, m2::PointD const & endPixelPos,
                                               bool isAutoZoom, AnimationClock::time_point requestedAt,
                                               double maxScaleDuration)
{
  if (IsFlightNeeded(startScreen, endScreen))
    return MapAnimation(MapAnimation::Type::Camera, GetMoveStage(startScreen, endScreen), requestedAt);

  auto const stage = GetFollowStage(startScreen, userPos, endPixelPos, endScreen.GetScale(), endScreen.GetAngle(),
                                    isAutoZoom, maxScaleDuration);
  if (stage.GetDuration() == 0.0)
    return {};
  return MapAnimation(MapAnimation::Type::Camera, stage, requestedAt, true /* syncArrow */);
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
