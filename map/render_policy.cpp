#include "../base/SRC_FIRST.hpp"

#include "render_policy.hpp"
#include "window_handle.hpp"

RenderPolicy::RenderPolicy(shared_ptr<WindowHandle> const & windowHandle, TRenderFn const & renderFn)
  : m_bgColor(0xEE, 0xEE, 0xDD, 0xFF),
    m_windowHandle(windowHandle),
    m_renderFn(renderFn)
{}

yg::Color const & RenderPolicy::bgColor() const
{
  return m_bgColor;
}

shared_ptr<yg::ResourceManager> const & RenderPolicy::resourceManager() const
{
  return m_resourceManager;
}

shared_ptr<WindowHandle> const & RenderPolicy::windowHandle() const
{
  return m_windowHandle;
}

RenderPolicy::TRenderFn RenderPolicy::renderFn() const
{
  return m_renderFn;
}

void RenderPolicy::Initialize(shared_ptr<yg::gl::RenderContext> const &,
                              shared_ptr<yg::ResourceManager> const & resourceManager)
{
  m_resourceManager = resourceManager;
}

m2::RectI const RenderPolicy::OnSize(int w, int h)
{
  return m2::RectI(0, 0, w, h);
}

void RenderPolicy::StartDrag(m2::PointD const &, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::DoDrag(m2::PointD const &, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StopDrag(m2::PointD const &, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StartScale(m2::PointD const &, m2::PointD const &, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::DoScale(m2::PointD const &, m2::PointD const &, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StopScale(m2::PointD const &, m2::PointD const &, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::BeginFrame()
{}

void RenderPolicy::EndFrame()
{}
