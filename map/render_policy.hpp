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

  typedef function<void(shared_ptr<PaintEvent>, ScreenBase const &, m2::RectD const &, int)> TRenderFn;

private:

  yg::Color m_bgColor;
  shared_ptr<yg::ResourceManager> m_resourceManager;
  shared_ptr<WindowHandle> m_windowHandle;
  TRenderFn m_renderFn;

protected:

  yg::Color const & bgColor() const;
  shared_ptr<yg::ResourceManager> const & resourceManager() const;
  shared_ptr<WindowHandle> const & windowHandle() const;
  TRenderFn renderFn() const;

public:

  /// constructor
  RenderPolicy(shared_ptr<WindowHandle> const & windowHandle, TRenderFn const & renderFn);
  /// drawing single frame
  virtual void DrawFrame(shared_ptr<PaintEvent> const & paintEvent, ScreenBase const & currentScreen) = 0;
  /// processing resize request
  virtual void OnSize(int w, int h) = 0;
  /// initialize render policy
  virtual void Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                          shared_ptr<yg::ResourceManager> const & resourceManager) = 0;
};
