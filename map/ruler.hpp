#pragma once

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../yg/overlay_element.hpp"
#include "../yg/font_desc.hpp"

namespace yg
{
  namespace gl
  {
    class Screen;
    class OverlayRenderer;
  }
}

class Ruler : public yg::OverlayElement
{
private:

  /// @todo Remove this variables. All this stuff are constants
  /// (get values from Framework constructor)
  unsigned m_minPxWidth;
  unsigned m_maxPxWidth;

  double m_minMetersWidth;
  double m_maxMetersWidth;
  //@}

  double m_visualScale;

  yg::FontDesc m_fontDesc;
  ScreenBase m_screen;

  /// Current diff in units between two endpoints of the ruler.
  /// @todo No need to store it here. It's calculated once for calculating ruler's m_path.
  double m_metresDiff;

  string m_scalerText;

  /// @todo Make static array with 2 elements.
  vector<m2::PointD> m_path;

  m2::RectD m_boundRect;

  typedef OverlayElement base_t;

  mutable vector<m2::AnyRectD> m_boundRects;

  typedef double (*ConversionFn)(double);
  ConversionFn m_conversionFn;

  int m_currSystem;
  void CalcMetresDiff(double v);

  bool m_isInitialized;
  bool m_hasPendingUpdate;

public:
  void update(); //< update internal params after some other params changed.

  struct Params : public base_t::Params
  {
  };

  Ruler(Params const & p);
  void setup();

  void setScreen(ScreenBase const & screen);
  ScreenBase const & screen() const;

  void setMinPxWidth(unsigned minPxWidth);
  void setMinMetersWidth(double v);
  void setMaxMetersWidth(double v);
  void setVisualScale(double visualScale);
  void setFontDesc(yg::FontDesc const & fontDesc);

  vector<m2::AnyRectD> const & boundRects() const;

  void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  int visualRank() const;
  yg::OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
};
