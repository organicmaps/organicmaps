#pragma once

#include "drape_frontend/animation/animation.hpp"

#include "geometry/screenbase.hpp"

#include <optional>

namespace df
{
class MapAnimation : public Animation
{
public:
  enum class Type
  {
    Camera,
    Pan,
    Zoom,
    Fling
  };

  struct Pose
  {
    m2::PointD m_position;
    double m_scale;
    double m_angle;
  };

  // Position, scale and angle change linearly, each during its own part of the animation.
  struct Stage
  {
    enum class Path
    {
      // The screen center moves along a line.
      Line,
      // The screen center slows down exponentially along a line.
      Fling,
      // The position under m_pixel moves along a line on the map from the start position to the end one, as the arrow
      // moves, while the map scales and rotates around it.
      Follow,
      // The map zooms out to m_flightScale while the end position starts to move to the screen center along a line
      // on the screen, and the map zooms to the end scale on arrival, see kFlyViewports.
      Fly
    };

    double GetDuration() const { return std::max({m_moveDuration, m_scaleDuration, m_rotateDuration}); }

    Path m_path = Path::Line;
    Pose m_start;
    Pose m_end;
    m2::PointD m_pixel{};
    double m_flightScale = 0.0;
    double m_moveDuration = 0.0;
    double m_scaleDuration = 0.0;
    double m_rotateDuration = 0.0;
  };

  // A syncArrow animation carries the arrow at m_pixel of a Follow.
  MapAnimation(Type type, Stage const & stage, AnimationClock::time_point requestedAt, bool syncArrow = false);

  Type GetType() const { return m_type; }
  void Apply(ScreenBase & screen) const;
  void ApplyTarget(ScreenBase & screen) const;
  void SyncArrow(double & moveDuration, double & rotateDuration) const;
  double GetRemainingScaleTime(AnimationClock::time_point now) const;

private:
  static void Apply(Stage const & stage, double elapsed, ScreenBase & screen);

  Type m_type;
  Stage m_stage;
  bool m_syncArrow;
};

bool IsAnimationAllowed(double duration);
double GetMoveDuration(double globalDistance, m2::RectD const & viewportRect, double scale);
double GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & screen);
double GetScaleDuration(double startScale, double endScale, bool isAutoZoom);
double GetRotateDuration(double startAngle, double endAngle);

// Moves, zooms and rotates the camera linearly within the time limit, or flies to a more distant screen. The scale
// of a move changes within maxScaleDuration.
std::optional<MapAnimation> GetScreenAnimation(MapAnimation::Type type, ScreenBase const & startScreen,
                                               ScreenBase const & endScreen, AnimationClock::time_point requestedAt,
                                               double maxScaleDuration = kMaxAnimationTimeSec);
// Brings userPos to endPixelPos of endScreen, or flies to a distant endScreen. The scale of a follow changes within
// maxScaleDuration.
std::optional<MapAnimation> GetFollowAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen,
                                               m2::PointD const & userPos, m2::PointD const & endPixelPos,
                                               bool isAutoZoom, AnimationClock::time_point requestedAt,
                                               double maxScaleDuration = kMaxAnimationTimeSec);
// Scales the map to endScreen around pxScaleCenter.
MapAnimation GetScaleAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen,
                               m2::PointD const & pxScaleCenter, AnimationClock::time_point requestedAt);
}  // namespace df
