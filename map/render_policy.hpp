#pragma once

#include "drawer_yg.hpp"

#include "../yg/color.hpp"

#include "../std/function.hpp"
#include "../std/shared_ptr.hpp"

#include "../geometry/rect2d.hpp"

class PaintEvent;
class ScreenBase;
class VideoTimer;

namespace yg
{
  namespace gl
  {
    class RenderContext;
  }

  class ResourceManager;
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

protected:

  yg::Color m_bgColor;
  shared_ptr<yg::ResourceManager> m_resourceManager;
  shared_ptr<yg::gl::RenderContext> m_primaryRC;
  shared_ptr<WindowHandle> m_windowHandle;
  shared_ptr<DrawerYG> m_drawer;
  TRenderFn m_renderFn;
  TEmptyModelFn m_emptyModelFn;
  bool m_doSupportRotation;
  bool m_doForceUpdate;
  m2::AnyRectD m_invalidRect;

public:

  /// constructor
  RenderPolicy(shared_ptr<yg::gl::RenderContext> const & primaryRC, bool doSupportRotation, size_t idCacheSize);
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
  virtual void SetEmptyModelFn(TEmptyModelFn emptyModelFn);

  bool DoSupportRotation() const;
  virtual bool IsTiling() const;

  virtual bool NeedRedraw() const;
  virtual bool IsEmptyModel() const;
  virtual int  GetDrawScale(ScreenBase const & s) const;

  bool DoForceUpdate() const;
  void SetForceUpdate(bool flag);

  void SetInvalidRect(m2::AnyRectD const & glbRect);
  m2::AnyRectD const & GetInvalidRect() const;

  shared_ptr<DrawerYG> const & GetDrawer() const;
  shared_ptr<WindowHandle> const & GetWindowHandle() const;

  virtual size_t ScaleEtalonSize() const;
};

RenderPolicy * CreateRenderPolicy(VideoTimer * videoTimer,
                                  bool useDefaultFB,
                                  yg::ResourceManager::Params const & rmParams,
                                  shared_ptr<yg::gl::RenderContext> const & primaryRC);

