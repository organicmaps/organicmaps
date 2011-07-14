#include "../base/SRC_FIRST.hpp"

#include "render_policy.hpp"

RenderPolicy::RenderPolicy(shared_ptr<WindowHandle> const & windowHandle, render_fn_t const & renderFn)
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

RenderPolicy::render_fn_t RenderPolicy::renderFn() const
{
  return m_renderFn;
}

void RenderPolicy::initialize(shared_ptr<yg::gl::RenderContext> const &,
                              shared_ptr<yg::ResourceManager> const & resourceManager)
{
  m_resourceManager = resourceManager;
}
