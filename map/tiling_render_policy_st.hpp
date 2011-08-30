#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile_cache.hpp"

#include "../yg/info_layer.hpp"

#include "../std/shared_ptr.hpp"

#include "../geometry/screenbase.hpp"

class DrawerYG;
class WindowHandle;

class TilingRenderPolicyST : public RenderPolicy
{
private:

  shared_ptr<DrawerYG> m_tileDrawer;
  ScreenBase m_tileScreen;

  yg::InfoLayer m_infoLayer;

  TileCache m_tileCache;
  Tiler m_tiler;

public:

  TilingRenderPolicyST(shared_ptr<WindowHandle> const & windowHandle,
                       RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & renderContext,
                  shared_ptr<yg::ResourceManager> const & resourceManager);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent, ScreenBase const & screenBase);
};
