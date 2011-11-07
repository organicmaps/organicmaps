#pragma once

#include "../yg/color.hpp"

#include "../std/function.hpp"
#include "../std/shared_ptr.hpp"

#include "../geometry/rect2d.hpp"

class PaintEvent;
class ScreenBase;

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

  typedef function<void(shared_ptr<PaintEvent>, ScreenBase const &, m2::RectD const &, m2::RectD const &, int)> TRenderFn;

private:

  yg::Color m_bgColor;
  shared_ptr<yg::ResourceManager> m_resourceManager;
  shared_ptr<WindowHandle> m_windowHandle;
  TRenderFn m_renderFn;
  bool m_doSupportRotation;

protected:

  yg::Color const & bgColor() const;
  shared_ptr<yg::ResourceManager> const & resourceManager() const;
  shared_ptr<WindowHandle> const & windowHandle() const;
  TRenderFn renderFn() const;

public:

  /// constructor
  RenderPolicy(shared_ptr<WindowHandle> const & windowHandle, TRenderFn const & renderFn, bool doSupportRotation);
  virtual ~RenderPolicy() {}
  /// starting frame
  virtual void BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
  /// drawing single frame
  virtual void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s) = 0;
  /// ending frame
  virtual void EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
  /// processing resize request
  virtual m2::RectI const OnSize(int w, int h);
  /// initialize render policy
  virtual void Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                          shared_ptr<yg::ResourceManager> const & resourceManager) = 0;

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

  bool DoSupportRotation() const;

  bool NeedRedraw() const;
};
