#pragma once

#include "drawer_yg.hpp"

#include "../yg/color.hpp"

#include "../std/function.hpp"
#include "../std/shared_ptr.hpp"

#include "../geometry/rect2d.hpp"

class PaintEvent;
class ScreenBase;
class VideoTimer;

namespace anim
{
  class Controller;
}

namespace yg
{
  namespace gl
  {
    class RenderContext;
  }

  class GlyphCache;
  class ResourceManager;
}

namespace anim
{
  class Controller;
  class Task;
}

class WindowHandle;

class RenderPolicy
{
public:

  typedef function<void(shared_ptr<PaintEvent>,
                        ScreenBase const &,
                        m2::RectD const &,
                        m2::RectD const &,
                        int,
                        bool)> TRenderFn;

  typedef function<bool (m2::PointD const &)> TEmptyModelFn;
  typedef function<string (m2::PointD const &)> TCountryNameFn;

protected:

  yg::Color m_bgColor;
  shared_ptr<yg::ResourceManager> m_resourceManager;
  shared_ptr<yg::gl::RenderContext> m_primaryRC;
  shared_ptr<WindowHandle> m_windowHandle;
  shared_ptr<DrawerYG> m_drawer;
  TRenderFn m_renderFn;
  TCountryNameFn m_countryNameFn;
  bool m_doSupportRotation;
  bool m_doForceUpdate;
  m2::AnyRectD m_invalidRect;
  double m_visualScale;
  string m_skinName;
  anim::Controller * m_controller;
  shared_ptr<yg::Overlay> m_overlay;

  void SetOverlay(shared_ptr<yg::Overlay> const & overlay);

public:

  struct Params
  {
    VideoTimer * m_videoTimer;
    bool m_useDefaultFB;
    yg::ResourceManager::Params m_rmParams;
    shared_ptr<yg::gl::RenderContext> m_primaryRC;
    double m_visualScale;
    string m_skinName;
    size_t m_screenWidth;
    size_t m_screenHeight;
  };

  /// constructor
  RenderPolicy(Params const & p,
               bool doSupportRotation,
               size_t idCacheSize);
  /// destructor
  virtual ~RenderPolicy();
  /// starting frame
  virtual void BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
  /// drawing single frame
  virtual void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s) = 0;
  /// ending frame
  virtual void EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
  /// processing resize request
  virtual m2::RectI const OnSize(int w, int h);
  /// reacting on navigation actions
  /// @{
  virtual void StartDrag();
  virtual void DoDrag();
  virtual void StopDrag();

  virtual void StartScale();
  virtual void DoScale();
  virtual void StopScale();

  virtual void StartRotate(double a, double timeInSec);
  virtual void DoRotate(double a, double timeInSec);
  virtual void StopRotate(double a, double timeInSec);
  /// @}

  /// the start point of rendering in renderpolicy.
  virtual void SetRenderFn(TRenderFn renderFn);
  virtual void SetCountryNameFn(TCountryNameFn countryNameFn);
  void SetAnimController(anim::Controller * controller);

  bool DoSupportRotation() const;
  virtual bool IsTiling() const;

  virtual bool NeedRedraw() const;
  virtual bool IsEmptyModel() const;
  virtual string const GetCountryName() const;
  virtual int  GetDrawScale(ScreenBase const & s) const;

  bool DoForceUpdate() const;
  void SetForceUpdate(bool flag);

  bool IsAnimating() const;

  void SetInvalidRect(m2::AnyRectD const & glbRect);
  m2::AnyRectD const & GetInvalidRect() const;

  shared_ptr<DrawerYG> const & GetDrawer() const;
  shared_ptr<WindowHandle> const & GetWindowHandle() const;
  yg::GlyphCache * GetGlyphCache() const;

  virtual size_t ScaleEtalonSize() const;

  double VisualScale() const;
  string const & SkinName() const;

  /// Benchmarking protocol
  virtual int InsertBenchmarkFence();
  virtual void JoinBenchmarkFence(int fenceID);

  virtual shared_ptr<yg::Overlay> const GetOverlay() const;
};

RenderPolicy * CreateRenderPolicy(RenderPolicy::Params const & params);
