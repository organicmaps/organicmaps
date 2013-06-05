#include "../base/SRC_FIRST.hpp"

#include "render_policy.hpp"
#include "window_handle.hpp"
#include "tiling_render_policy_st.hpp"
#include "tiling_render_policy_mt.hpp"

#include "../anim/controller.hpp"
#include "../anim/task.hpp"

#include "../graphics/opengl/opengl.hpp"
#include "../graphics/opengl/gl_render_context.hpp"

#include "../indexer/scales.hpp"
#include "../indexer/drawing_rules.hpp"

#include "../platform/video_timer.hpp"
#include "../platform/settings.hpp"
#include "../platform/platform.hpp"

RenderPolicy::~RenderPolicy()
{
  LOG(LDEBUG, ("clearing cached drawing rules"));
  drule::rules().ClearCaches();
  if (m_primaryRC && m_resourceManager)
    m_primaryRC->endThreadDrawing(m_resourceManager->guiThreadSlot());
}

RenderPolicy::RenderPolicy(Params const & p,
                           bool doSupportRotation,
                           size_t idCacheSize)
  : m_bgColor(0xEE, 0xEE, 0xDD, 0xFF),
    m_primaryRC(p.m_primaryRC),
    m_doSupportRotation(doSupportRotation),
    m_doForceUpdate(false),
    m_density(p.m_density),
    m_visualScale(graphics::visualScale(p.m_density)),
    m_skinName(p.m_skinName)
{
  LOG(LDEBUG, ("each BaseRule will hold up to", idCacheSize, "cached values"));
  drule::rules().ResizeCaches(idCacheSize);

  graphics::gl::InitExtensions();
  graphics::gl::CheckExtensionSupport();
}

void RenderPolicy::InitCacheScreen()
{
  graphics::Screen::Params cp;

  cp.m_doUnbindRT = false;
  cp.m_threadSlot = m_resourceManager->guiThreadSlot();
  cp.m_storageType = graphics::ETinyStorage;
  cp.m_textureType = graphics::ESmallTexture;
  cp.m_isSynchronized = false;
  cp.m_resourceManager = m_resourceManager;
  cp.m_renderContext = m_primaryRC;

  m_cacheScreen = make_shared_ptr(new graphics::Screen(cp));
}

m2::RectI const RenderPolicy::OnSize(int w, int h)
{
  m_cacheScreen->onSize(w, h);
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
{
  /// processing animations at the beginning of the frame.
  /// it's crucial as in this function could happen transition from
  /// animating to non-animating state which should be properly handled
  /// in the following RenderPolicy::DrawFrame call.
  m_controller->PerformStep();
}

void RenderPolicy::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
}

bool RenderPolicy::DoSupportRotation() const
{
  return m_doSupportRotation;
}

bool RenderPolicy::NeedRedraw() const
{
  return m_windowHandle->needRedraw()
      || IsAnimating();
}

bool RenderPolicy::IsAnimating() const
{
  return (m_controller->HasVisualTasks()
      || (m_controller->LockCount() > 0)
      || (m_controller->IsVisuallyPreWarmed()));
}

bool RenderPolicy::IsTiling() const
{
  return false;
}

shared_ptr<Drawer> const & RenderPolicy::GetDrawer() const
{
  return m_drawer;
}

shared_ptr<WindowHandle> const & RenderPolicy::GetWindowHandle() const
{
  return m_windowHandle;
}

graphics::GlyphCache * RenderPolicy::GetGlyphCache() const
{
  return m_resourceManager->glyphCache(m_resourceManager->guiThreadSlot());
}

void RenderPolicy::SetRenderFn(TRenderFn renderFn)
{
  m_renderFn = renderFn;
}

void RenderPolicy::SetCountryIndexFn(TCountryIndexFn const & fn)
{
  m_countryIndexFn = fn;
}

bool RenderPolicy::DoForceUpdate() const
{
  return m_doForceUpdate;
}

void RenderPolicy::SetForceUpdate(bool flag)
{
  m_doForceUpdate = flag;
}

void RenderPolicy::SetInvalidRect(m2::AnyRectD const & glbRect)
{
  m_invalidRect = glbRect;
}

m2::AnyRectD const & RenderPolicy::GetInvalidRect() const
{
  return m_invalidRect;
}

bool RenderPolicy::IsEmptyModel() const
{
  return false;
}

double RenderPolicy::VisualScale() const
{
  return m_visualScale;
}

graphics::EDensity RenderPolicy::Density() const
{
  return m_density;
}

string const & RenderPolicy::SkinName() const
{
  return m_skinName;
}

int RenderPolicy::InsertBenchmarkFence()
{
  return 0;
}

void RenderPolicy::JoinBenchmarkFence(int fenceID)
{
}

void RenderPolicy::SetAnimController(anim::Controller * controller)
{
  m_controller = controller;
}

void RenderPolicy::FrameLock()
{
  LOG(LWARNING, ("unimplemented method called."));
}

void RenderPolicy::FrameUnlock()
{
  LOG(LWARNING, ("unimplemented method called"));
}

graphics::Overlay * RenderPolicy::FrameOverlay() const
{
  LOG(LWARNING, ("unimplemented method called"));
  return NULL;
}

graphics::Color const RenderPolicy::GetBgColor() const
{
  return m_bgColor;
}

shared_ptr<graphics::Screen> const & RenderPolicy::GetCacheScreen() const
{
  return m_cacheScreen;
}

RenderPolicy * CreateRenderPolicy(RenderPolicy::Params const & params)
{
#ifdef OMIM_OS_ANDROID
    return new TilingRenderPolicyST(params);
#endif
#ifdef OMIM_OS_IPHONE
    return new TilingRenderPolicyMT(params);
#endif
#ifdef OMIM_OS_DESKTOP
    return new TilingRenderPolicyST(params);
#endif
}
