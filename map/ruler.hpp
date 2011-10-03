#pragma once

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../yg/overlay_element.hpp"
#include "../yg/font_desc.hpp"

namespace yg
{
  class StylesCache;

  namespace gl
  {
    class Screen;
    class OverlayRenderer;
  }
}

class Ruler : public yg::OverlayElement
{
private:

  unsigned m_minPxWidth;
  unsigned m_maxPxWidth;

  double m_minUnitsWidth;
  double m_maxUnitsWidth;
  double m_visualScale;

  yg::FontDesc m_fontDesc;
  ScreenBase m_screen;

  float m_unitsDiff; //< current diff in units between two endpoints of the ruler.
  string m_scalerText;
  vector<m2::PointD> m_path;

  m2::RectD m_boundRect;

  unsigned ceil(double unitsDiff);

  typedef OverlayElement base_t;

  mutable vector<m2::AnyRectD> m_boundRects;

public:

  void update(); //< update internal params after some other params changed.

  struct Params : public base_t::Params
  {
  };

  Ruler(Params const & p);

  void setScreen(ScreenBase const & screen);
  ScreenBase const & screen() const;

  void setMinPxWidth(unsigned minPxWidth);
  void setMinUnitsWidth(double minUnitsWidth);
  void setMaxUnitsWidth(double maxUnitsWidth);
  void setVisualScale(double visualScale);
  void setFontDesc(yg::FontDesc const & fontDesc);

  vector<m2::AnyRectD> const & boundRects() const;

  void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  void map(yg::StylesCache * stylesCache) const;
  bool find(yg::StylesCache * stylesCache) const;
  void fillUnpacked(yg::StylesCache * stylesCache, vector<m2::PointU> & v) const;

  int visualRank() const;
  yg::OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
};
