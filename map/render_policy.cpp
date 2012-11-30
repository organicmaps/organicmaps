#include "../base/SRC_FIRST.hpp"

#include "render_policy.hpp"
#include "window_handle.hpp"
#include "test_render_policy.hpp"
#include "simple_render_policy.hpp"
#include "tiling_render_policy_st.hpp"
#include "tiling_render_policy_mt.hpp"

#include "../anim/controller.hpp"
#include "../anim/task.hpp"

#include "../graphics/opengl/opengl.hpp"
#include "../graphics/opengl/gl_render_context.hpp"
#include "../graphics/skin.hpp"

#include "../indexer/scales.hpp"
#include "../indexer/drawing_rules.hpp"

#include "../platform/video_timer.hpp"
#include "../platform/settings.hpp"
#include "../platform/platform.hpp"

RenderPolicy::~RenderPolicy()
{
  LOG(LDEBUG, ("clearing cached drawing rules"));
  drule::rules().ClearCaches();
  graphics::gl::FinalizeThread();
}

RenderPolicy::RenderPolicy(Params const & p,
                           bool doSupportRotation,
                           size_t idCacheSize)
  : m_bgColor(0xEE, 0xEE, 0xDD, 0xFF),
    m_primaryRC(p.m_primaryRC),
    m_doSupportRotation(doSupportRotation),
    m_doForceUpdate(false),
    m_visualScale(p.m_visualScale),
    m_skinName(p.m_skinName)
{
  LOG(LDEBUG, ("each BaseRule will hold up to", idCacheSize, "cached values"));
  drule::rules().ResizeCaches(idCacheSize);

  graphics::gl::InitExtensions();
  graphics::gl::InitializeThread();
  graphics::gl::CheckExtensionSupport();

  m_primaryRC->startThreadDrawing();
}

void RenderPolicy::InitCacheScreen()
{
  graphics::Screen::Params cp;

  cp.m_doUnbindRT = false;
  cp.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  cp.m_useGuiResources = true;
  cp.m_isSynchronized = false;
  cp.m_resourceManager = m_resourceManager;

  m_cacheScreen = make_shared_ptr(new graphics::Screen(cp));

  m_cacheScreen->setSkin(m_skin);
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
  return m_resourceManager->glyphCache(m_resourceManager->guiThreadGlyphCacheID());
}

void RenderPolicy::SetRenderFn(TRenderFn renderFn)
{
  m_renderFn = renderFn;
}

void RenderPolicy::SetCountryNameFn(TCountryNameFn countryNameFn)
{
  m_countryNameFn = countryNameFn;
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

string const RenderPolicy::GetCountryName() const
{
  return string();
}

int RenderPolicy::GetDrawScale(ScreenBase const & s) const
{
  m2::PointD textureCenter(s.PixelRect().Center());
  m2::RectD glbRect;

  unsigned scaleEtalonSize = GetPlatform().ScaleEtalonSize();
  s.PtoG(m2::RectD(textureCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                   textureCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
                   glbRect);
  return scales::GetScaleLevel(glbRect);
}

size_t RenderPolicy::ScaleEtalonSize() const
{
  return GetPlatform().ScaleEtalonSize();
}

double RenderPolicy::VisualScale() const
{
  return m_visualScale;
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

void RenderPolicy::SetOverlay(shared_ptr<graphics::Overlay> const & overlay)
{
  m_overlay = overlay;
}

shared_ptr<graphics::Overlay> const RenderPolicy::GetOverlay() const
{
  return m_overlay;
}

graphics::Color const RenderPolicy::GetBgColor() const
{
  return m_bgColor;
}

shared_ptr<graphics::Screen> const & RenderPolicy::GetCacheScreen() const
{
  return m_cacheScreen;
}

void RenderPolicy::SetSkin(shared_ptr<graphics::Skin> const & skin)
{
  m_skin = skin;
}

shared_ptr<graphics::Skin> const & RenderPolicy::GetSkin() const
{
  return m_skin;
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
