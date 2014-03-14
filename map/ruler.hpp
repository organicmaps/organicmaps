#pragma once

#include "../gui/element.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/any_rect2d.hpp"

#include "../graphics/display_list.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/list.hpp"

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
  typedef gui::Element base_t;

  mutable vector<m2::AnyRectD> m_boundRects;

  class RulerFrame
  {
  public:
    typedef function<void (bool isVisible, RulerFrame *)> frame_end_fn;

    RulerFrame(Framework & f,
               frame_end_fn const & fn,
               double depth);

    RulerFrame(RulerFrame const & other, const frame_end_fn & fn);
    ~RulerFrame();

    bool IsValid() const;

    void Cache(const string & text, const graphics::FontDesc & f);
    void Purge();
    bool IsHidingAnim() const;
    bool IsAnimActive() const;
    void SetScale(double scale);
    double GetScale() const;
    void SetOrgPoint(m2::PointD const & org);
    m2::PointD const & GetOrgPoint() const;

    void ShowAnimate(bool needPause);
    void HideAnimate(bool needPause);
    void Draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

  private:
    void CreateAnim(double startAlfa, double endAlfa,
                    double timeInterval, double timeOffset,
                    bool isVisibleAtEnd);
    float GetCurrentAlfa();
    void AnimEnded(bool isVisible);

  private:
    Framework & m_f;
    shared_ptr<graphics::DisplayList> m_dl;
    shared_ptr<graphics::DisplayList> m_textDL;
    double m_scale;
    double m_depth;
    m2::PointD m_orgPt;
    frame_end_fn m_callback;

    shared_ptr<anim::Task> m_frameAnim;
  };

private:
  int m_currentRangeIndex;
  int m_currSystem;
  double CalcMetresDiff(double value);
  void UpdateText(const string & text);

  void MainFrameAnimEnded(bool isVisible, RulerFrame * frame);
  void AnimFrameAnimEnded(bool isVisible, RulerFrame * frame);

  RulerFrame * GetMainFrame();
  RulerFrame * GetMainFrame() const;
  RulerFrame * m_mainFrame;
  RulerFrame * m_animFrame;

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

  vector<m2::AnyRectD> const & boundRects() const;
  void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

  void update();
  void layout();
  void cache();
  void purge();
};
