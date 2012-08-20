#pragma once

#include "../gui/element.hpp"

#include "../yg/color.hpp"

#include "../geometry/any_rect2d.hpp"

#include "../std/shared_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class DisplayList;
  }
}

class RotateScreenTask;
class Framework;

/// Compass Arrow, which shows up when the screen is rotated,
/// and rotates screen back to straight orientation when beeing pressed
class CompassArrow : public gui::Element
{
private:

  typedef gui::Element base_t;

  unsigned m_arrowWidth;
  unsigned m_arrowHeight;
  yg::Color m_northColor;
  yg::Color m_southColor;
  double m_angle;

  shared_ptr<yg::gl::DisplayList> m_displayList;
  shared_ptr<RotateScreenTask> m_rotateScreenTask;

  mutable vector<m2::AnyRectD> m_boundRects;

  math::Matrix<double, 3, 3> m_drawM;

  Framework * m_framework;

  void cache();
  void purge();

public:

  struct Params : public base_t::Params
  {
    unsigned m_arrowWidth;
    unsigned m_arrowHeight;
    yg::Color m_northColor;
    yg::Color m_southColor;
    Framework * m_framework;
  };

  CompassArrow(Params const & p);

  void setAngle(double angle);

  vector<m2::AnyRectD> const & boundRects() const;
  void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  bool onTapEnded(m2::PointD const & pt);

  bool hitTest(m2::PointD const & pt) const;
};
