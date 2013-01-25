#pragma once

#include "../gui/element.hpp"
#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../graphics/font_desc.hpp"

namespace graphics
{
  namespace gl
  {
    class Screen;
    class OverlayRenderer;
  }
}

class Ruler : public gui::Element
{
private:

  /// @todo Remove this variables. All this stuff are constants
  /// (get values from Framework constructor)
  unsigned m_minPxWidth;
  unsigned m_maxPxWidth;

  double m_minMetersWidth;
  double m_maxMetersWidth;
  //@}

  ScreenBase m_screen;

  /// Current diff in units between two endpoints of the ruler.
  /// @todo No need to store it here. It's calculated once for calculating ruler's m_path.
  double m_metresDiff;

  string m_scalerText;

  /// @todo Make static array with 2 elements.
  vector<m2::PointD> m_path;

  m2::RectD m_boundRect;

  typedef gui::Element base_t;

  mutable vector<m2::AnyRectD> m_boundRects;

  typedef double (*ConversionFn)(double);
  ConversionFn m_conversionFn;

  int m_currSystem;
  void CalcMetresDiff(double v);

public:

  typedef base_t::Params Params;

  Ruler(Params const & p);

  void setScreen(ScreenBase const & screen);
  ScreenBase const & screen() const;

  void setMinPxWidth(unsigned minPxWidth);
  void setMinMetersWidth(double v);
  void setMaxMetersWidth(double v);
  void setFontDesc(graphics::FontDesc const & fontDesc);

  vector<m2::AnyRectD> const & boundRects() const;

  void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  void layout();
  void update();
};
