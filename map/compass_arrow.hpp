#pragma once

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"


namespace anim
{
  class Task;
}

class Framework;

/// Compass Arrow, which shows up when the screen is rotated,
/// and rotates screen back to straight orientation when beeing pressed
class CompassArrow
{
  double m_angle;
  shared_ptr<anim::Task> m_animTask;

  void AlfaAnimEnded(bool isVisible);
  bool IsHidingAnim() const;
  float GetCurrentAlfa() const;
  void CreateAnim(double startAlfa, double endAlfa, double timeInterval, double timeOffset, bool isVisibleAtEnd);

  Framework * m_framework;
  bool isBaseVisible() const;

public:
  struct Params
  {
    Framework * m_framework;
    Params();
  };

  CompassArrow(Params const & p);

  void AnimateShow();
  void AnimateHide();

  void SetAngle(double angle);

  /// @name Override from graphics::Overlayelement and gui::Element.
  //@{

  bool isVisible() const;
  //bool hitTest(m2::PointD const & pt) const;

  void cache();
  void purge();

  //bool onTapEnded(m2::PointD const & pt);
  //@}
};
