#pragma once

#include "../std/shared_ptr.hpp"

#include "../gui/element.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"

#include "../graphics/display_list.hpp"

namespace graphics
{
  namespace gl
  {
    class OverlayRenderer;
  }
}

namespace gui
{
  class CachedTextView;
}

class Framework;

class Ruler : public gui::Element
{
private:

  /// @todo Remove this variables. All this stuff are constants
  /// (get values from Framework constructor)
  unsigned m_minPxWidth;

  double m_minMetersWidth;
  double m_maxMetersWidth;
  //@}

  /// Current diff in units between two endpoints of the ruler.
  /// @todo No need to store it here. It's calculated once for calculating ruler's m_path.
  double m_metresDiff;

  double m_cacheLength;
  double m_scaleKoeff;
  m2::PointD m_scalerOrg;

  m2::RectD m_boundRect;

  typedef gui::Element base_t;

  mutable vector<m2::AnyRectD> m_boundRects;

  typedef double (*ConversionFn)(double);
  ConversionFn m_conversionFn;

  int m_currSystem;
  void CalcMetresDiff(double v);

  shared_ptr<gui::CachedTextView> m_scaleText;
  scoped_ptr<graphics::DisplayList> m_dl;

  Framework * m_framework;

public:

  struct Params : public Element::Params
  {
    Framework * m_framework;
    Params();
  };

  Ruler(Params const & p);

  void setController(gui::Controller * controller);

  void setMinPxWidth(unsigned minPxWidth);
  void setMinMetersWidth(double v);
  void setMaxMetersWidth(double v);

  void setFont(gui::Element::EState state, graphics::FontDesc const & f);

  vector<m2::AnyRectD> const & boundRects() const;
  void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  void update();
  void layout();
  void cache();
  void purge();
};
