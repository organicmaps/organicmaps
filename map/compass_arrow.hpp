#pragma once

#include "../gui/element.hpp"

#include "../yg/color.hpp"
#include "../yg/display_list.hpp"

#include "../geometry/any_rect2d.hpp"

#include "../std/scoped_ptr.hpp"

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

  scoped_ptr<yg::gl::DisplayList> m_displayList;

  mutable vector<m2::AnyRectD> m_boundRects;

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

  void SetAngle(double angle);

  unsigned GetArrowHeight() const;

  vector<m2::AnyRectD> const & boundRects() const;
  void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  bool onTapEnded(m2::PointD const & pt);

  bool hitTest(m2::PointD const & pt) const;
};
