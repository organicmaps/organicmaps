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
    Follow,
    Pan,
    Zoom,
    Fling
  };

  MapAnimation(Type type, ScreenBase const & start, ScreenBase const & end, AnimationClock::time_point requestedAt,
               double duration, std::optional<m2::PointD> pixelAnchor = std::nullopt);

  Type GetType() const { return m_type; }
  void Apply(ScreenBase & screen) const { Apply(screen, GetProgress()); }
  void ApplyTarget(ScreenBase & screen) const { Apply(screen, 1.0); }

private:
  struct Pose
  {
    m2::PointD m_position;
    double m_scale;
    double m_angle;
  };

  static Pose GetPose(ScreenBase const & screen, std::optional<m2::PointD> const & pixelAnchor);
  void Apply(ScreenBase & screen, double progress) const;

  Type const m_type;
  std::optional<m2::PointD> const m_pixelAnchor;
  Pose const m_start;
  Pose const m_end;
};

bool IsWithinAnimationDistance(ScreenBase const & screen, m2::PointD const & from, m2::PointD const & to);
}  // namespace df
