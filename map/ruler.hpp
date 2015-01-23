#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/function.hpp"


namespace anim
{
  class Task;
}

class Framework;


class Ruler
{
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

    void Purge();
    bool IsHidingAnim() const;
    bool IsAnimActive() const;
    void SetScale(double scale);
    double GetScale() const;
    void SetOrgPoint(m2::PointD const & org);
    m2::PointD const & GetOrgPoint() const;

    void ShowAnimate(bool needPause);
    void HideAnimate(bool needPause);

  private:
    void CreateAnim(double startAlfa, double endAlfa,
                    double timeInterval, double timeOffset,
                    bool isVisibleAtEnd);
    float GetCurrentAlfa();
    void AnimEnded(bool isVisible);

  private:
    Framework & m_f;

    int m_textLengthInPx;
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

  unique_ptr<RulerFrame> m_mainFrame;
  unique_ptr<RulerFrame> m_animFrame;

  Framework * m_framework;

public:

  struct Params
  {
    Framework * m_framework;
    Params();
  };

  Ruler(Params const & p);

  void AnimateShow();
  void AnimateHide();

  /// @name Override from graphics::OverlayElement and gui::Element.
  //@{
  virtual m2::RectD GetBoundRect() const;

  void update();
  void layout();
  void cache();
  void purge();
  //@}

  int GetTextOffsetFromLine() const;
};
