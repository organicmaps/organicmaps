#include "../base/SRC_FIRST.hpp"

#include "render_policy_st.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"
#include "../yg/info_layer.hpp"

#include "../indexer/scales.hpp"
#include "../geometry/screenbase.hpp"

#include "../platform/platform.hpp"

RenderPolicyST::RenderPolicyST(shared_ptr<WindowHandle> const & wh,
                               RenderPolicy::TRenderFn const & renderFn)
  : RenderPolicy(wh, renderFn)
{}

void RenderPolicyST::Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                                shared_ptr<yg::ResourceManager> const & rm)
{
  RenderPolicy::Initialize(rc, rm);
}

void RenderPolicyST::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  int scaleEtalonSize = GetPlatform().ScaleEtalonSize();

  m2::RectD glbRect;
  m2::PointD pxCenter = s.PixelRect().Center();
  s.PtoG(m2::RectD(pxCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                   pxCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
         glbRect);

  shared_ptr<yg::InfoLayer> infoLayer(new yg::InfoLayer());

  e->drawer()->screen()->setInfoLayer(infoLayer);

  e->drawer()->SetVisualScale(GetPlatform().VisualScale());

  e->drawer()->screen()->clear(bgColor());
  renderFn()(e, s, s.GlobalRect(), scales::GetScaleLevel(glbRect));

  infoLayer->draw(e->drawer()->screen().get(), math::Identity<double, 3>());
  e->drawer()->screen()->resetInfoLayer();
}

void RenderPolicyST::OnSize(int w, int h)
{
}
