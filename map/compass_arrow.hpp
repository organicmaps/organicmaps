#pragma once

#include "../gui/element.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/unique_ptr.hpp"


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
  typedef gui::Element BaseT;

  mutable m2::PointI m_pixelSize;
  double m_angle;

  unique_ptr<graphics::DisplayList> m_dl;

  shared_ptr<anim::Task> m_animTask;

  void AlfaAnimEnded(bool isVisible);
  bool IsHidingAnim() const;
  float GetCurrentAlfa() const;
  void CreateAnim(double startAlfa, double endAlfa, double timeInterval, double timeOffset, bool isVisibleAtEnd);

  Framework * m_framework;
  graphics::Resource const * GetCompassResource() const;

  bool isBaseVisible() const;

public:
  struct Params : public BaseT::Params
  {
    Framework * m_framework;
    Params();
  };

  CompassArrow(Params const & p);

  void AnimateShow();
  void AnimateHide();

  void SetAngle(double angle);
  m2::PointI GetPixelSize() const;

  /// @name Override from graphics::Overlayelement and gui::Element.
  //@{
  virtual void GetMiniBoundRects(RectsT & rects) const;

  void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
  bool isVisible() const;
  bool roughHitTest(m2::PointD const & pt) const;
  bool hitTest(m2::PointD const & pt) const;

  void cache();
  void purge();

  bool onTapEnded(m2::PointD const & pt);
  //@}
};
