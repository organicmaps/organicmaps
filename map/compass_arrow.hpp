#pragma once

#include "../gui/element.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../std/shared_ptr.hpp"

namespace anim
{
  class Task;
}

namespace graphics
{
  class DisplayList;
  struct Resource;
}

class Framework;

/// Compass Arrow, which shows up when the screen is rotated,
/// and rotates screen back to straight orientation when beeing pressed
class CompassArrow : public gui::Element
{
private:
  typedef gui::Element base_t;

  double m_angle;

  graphics::DisplayList * m_displayList;

  shared_ptr<anim::Task> m_animTask;

  void AlfaAnimEnded(bool isVisible);
  bool IsHidingAnim() const;
  float GetCurrentAlfa() const;
  void CreateAnim(double startAlfa, double endAlfa, double timeInterval, double timeOffset, bool isVisibleAtEnd);

  mutable vector<m2::AnyRectD> m_boundRects;

  Framework * m_framework;
  graphics::Resource const * GetCompassResource() const;

  void cache();
  void purge();
  bool isBaseVisible() const;

public:

  struct Params : public base_t::Params
  {
    Framework * m_framework;
    Params();
  };

  CompassArrow(Params const & p);

  void AnimateShow();
  void AnimateHide();

  void SetAngle(double angle);
  m2::PointD GetPixelSize() const;

  vector<m2::AnyRectD> const & boundRects() const;
  void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
  virtual bool isVisible() const;

  bool onTapEnded(m2::PointD const & pt);

  bool roughHitTest(m2::PointD const & pt) const;
  bool hitTest(m2::PointD const & pt) const;
};
