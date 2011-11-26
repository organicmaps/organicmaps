#include "../base/SRC_FIRST.hpp"

#include "render_policy.hpp"

#include "../indexer/drawing_rules.hpp"

#include "../map/render_policy_st.hpp"
#include "../map/render_policy_mt.hpp"
#include "../map/tiling_render_policy_st.hpp"
#include "../map/tiling_render_policy_mt.hpp"
#include "../map/partial_render_policy.hpp"
#include "../map/benchmark_render_policy_mt.hpp"
#include "../map/benchmark_tiling_render_policy_mt.hpp"

#include "../yg/internal/opengl.hpp"

#include "../platform/video_timer.hpp"
#include "../platform/settings.hpp"

RenderPolicy::~RenderPolicy()
{
  LOG(LINFO, ("clearing cached drawing rules"));
  drule::rules().ClearCaches();
}

RenderPolicy::RenderPolicy(shared_ptr<yg::gl::RenderContext> const & primaryRC, bool doSupportRotation)
  : m_bgColor(0xEE, 0xEE, 0xDD, 0xFF),
    m_primaryRC(primaryRC),
    m_doSupportRotation(doSupportRotation),
    m_doForceUpdate(false)
{
  yg::gl::InitExtensions();
  yg::gl::CheckExtensionSupport();
}

m2::RectI const RenderPolicy::OnSize(int w, int h)
{
  m_drawer->onSize(w, h);
  return m2::RectI(0, 0, w, h);
}

void RenderPolicy::StartDrag()
{
  m_windowHandle->invalidate();
}

void RenderPolicy::DoDrag()
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StopDrag()
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StartScale()
{
  m_windowHandle->invalidate();
}

void RenderPolicy::DoScale()
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StopScale()
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StartRotate(double a, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::DoRotate(double a, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::StopRotate(double a, double)
{
  m_windowHandle->invalidate();
}

void RenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{}

void RenderPolicy::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{}

bool RenderPolicy::DoSupportRotation() const
{
  return m_doSupportRotation;
}

bool RenderPolicy::NeedRedraw() const
{
  return m_windowHandle->needRedraw();
}

bool RenderPolicy::IsTiling() const
{
  return false;
}

shared_ptr<DrawerYG> const & RenderPolicy::GetDrawer() const
{
  return m_drawer;
}

shared_ptr<WindowHandle> const & RenderPolicy::GetWindowHandle() const
{
  return m_windowHandle;
}

void RenderPolicy::SetRenderFn(TRenderFn renderFn)
{
  m_renderFn = renderFn;
}

bool RenderPolicy::DoForceUpdate() const
{
  return m_doForceUpdate;
}

void RenderPolicy::SetForceUpdate(bool flag)
{
  m_doForceUpdate = flag;
}

RenderPolicy * CreateRenderPolicy(VideoTimer * videoTimer,
                                  bool useDefaultFB,
                                  yg::ResourceManager::Params const & rmParams,
                                  shared_ptr<yg::gl::RenderContext> const & primaryRC)
{
  bool benchmarkingEnabled = false;
  Settings::Get("IsBenchmarking", benchmarkingEnabled);

  if (benchmarkingEnabled)
  {
    bool isBenchmarkingMT = false;
    Settings::Get("IsBenchmarkingMT", isBenchmarkingMT);

    if (isBenchmarkingMT)
      return new BenchmarkTilingRenderPolicyMT(videoTimer, useDefaultFB, rmParams, primaryRC);
    else
      return new BenchmarkRenderPolicyMT(videoTimer, useDefaultFB, rmParams, primaryRC);
  }
  else
  {
#ifdef OMIM_OS_ANDROID
    return new PartialRenderPolicy(videoTimer, useDefaultFB, rmParams, primaryRC);
#endif
#ifdef OMIM_OS_IPHONE
    return new RenderPolicyMT(videoTimer, useDefaultFB, rmParams, primaryRC);
#endif
#ifdef OMIM_OS_DESKTOP
    return new RenderPolicyMT(videoTimer, useDefaultFB, rmParams, primaryRC);
#endif
  }
}
