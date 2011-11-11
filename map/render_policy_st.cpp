#include "../base/SRC_FIRST.hpp"

#include "render_policy_st.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "../yg/info_layer.hpp"
#include "../yg/internal/opengl.hpp"

#include "../indexer/scales.hpp"
#include "../geometry/screenbase.hpp"

#include "../platform/platform.hpp"

RenderPolicyST::RenderPolicyST(VideoTimer * videoTimer,
                               DrawerYG::Params const & params,
                               shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, false)
{
  m_resourceManager = make_shared_ptr(new yg::ResourceManager(
      50000 * sizeof(yg::gl::Vertex),
      100000 * sizeof(unsigned short),
      15,
      5000 * sizeof(yg::gl::Vertex),
      10000 * sizeof(unsigned short),
      100,
      10 * sizeof(yg::gl::AuxVertex),
      10 * sizeof(unsigned short),
      50,
      512, 256,
      10,
      512, 256,
      5,
      "unicode_blocks.txt",
      "fonts_whitelist.txt",
      "fonts_blacklist.txt",
      2 * 1024 * 1024,
      1,
      yg::Rt8Bpp,
      !yg::gl::g_isBufferObjectsSupported));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p = params;

  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_isSynchronized = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);
}

void RenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  int scaleEtalonSize = GetPlatform().ScaleEtalonSize();

  m2::RectD glbRect;
  m2::PointD const pxCenter = s.PixelRect().Center();
  s.PtoG(m2::RectD(pxCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                   pxCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
         glbRect);

  shared_ptr<yg::InfoLayer> infoLayer(new yg::InfoLayer());

  e->drawer()->screen()->setInfoLayer(infoLayer);

  e->drawer()->screen()->clear(m_bgColor);

  m_renderFn(e, s, s.ClipRect(), s.ClipRect(), scales::GetScaleLevel(glbRect));

  infoLayer->draw(e->drawer()->screen().get(), math::Identity<double, 3>());
  e->drawer()->screen()->resetInfoLayer();
}
