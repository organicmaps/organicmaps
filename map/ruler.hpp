#pragma once

#include "../std/shared_ptr.hpp"

#include "../gui/element.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"

#include "../graphics/display_list.hpp"

namespace anim
{
  class Task;
}

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

  shared_ptr<anim::Task> m_rulerAnim;
  void AlfaAnimEnded(bool isVisible);
  bool IsHidingAnim() const;
  float GetCurrentAlfa() const;
  void CreateAnim(double startAlfa, double endAlfa, double timeInterval, double timeOffset, bool isVisibleAtEnd);

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

  graphics::DisplayList * m_dl;
  void PurgeMainDL();
  void CacheMainDL();

  graphics::DisplayList * m_textDL[2];
  void PurgeTextDL(int index);
  void UpdateText(const string & text);

  Framework * m_framework;

public:

  struct Params : public Element::Params
  {
    Framework * m_framework;
    Params();
  };

  Ruler(Params const & p);

  void AnimateShow();
  void AnimateHide();

  void setMinPxWidth(unsigned minPxWidth);
  void setMinMetersWidth(double v);
  void setMaxMetersWidth(double v);

  vector<m2::AnyRectD> const & boundRects() const;
  void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  void update();
  void layout();
  void cache();
  void purge();
};
