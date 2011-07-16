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

  typedef function<void(shared_ptr<PaintEvent>, ScreenBase const &, m2::RectD const &, int)> render_fn_t;

private:

  yg::Color m_bgColor;
  shared_ptr<yg::ResourceManager> m_resourceManager;
  shared_ptr<WindowHandle> m_windowHandle;
  render_fn_t m_renderFn;

protected:

  yg::Color const & bgColor() const;
  shared_ptr<yg::ResourceManager> const & resourceManager() const;
  shared_ptr<WindowHandle> const & windowHandle() const;
  render_fn_t renderFn() const;

public:

  /// constructor
  RenderPolicy(shared_ptr<WindowHandle> const & windowHandle, render_fn_t const & renderFn);
  /// drawing single frame
  virtual void drawFrame(shared_ptr<PaintEvent> const & paintEvent, ScreenBase const & currentScreen) = 0;
  /// processing resize request
  virtual void onSize(int w, int h) = 0;
  /// initialize render policy
  virtual void initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                          shared_ptr<yg::ResourceManager> const & resourceManager) = 0;
};
