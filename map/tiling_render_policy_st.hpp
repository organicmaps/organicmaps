#pragma once

#include "render_policy.hpp"
#include "drawer_yg.hpp"
#include "tiler.hpp"
#include "tile_cache.hpp"

#include "../yg/info_layer.hpp"

#include "../std/shared_ptr.hpp"

#include "../geometry/screenbase.hpp"

class WindowHandle;
class VideoTimer;

namespace yg
{
  namespace gl
  {
    class RenderContext;
  }
}

class TilingRenderPolicyST : public RenderPolicy
{
private:

  shared_ptr<DrawerYG> m_tileDrawer;
  ScreenBase m_tileScreen;

  yg::InfoLayer m_infoLayer;

  TileCache m_tileCache;
  Tiler m_tiler;

public:

  TilingRenderPolicyST(VideoTimer * videoTimer,
                       DrawerYG::Params const & params,
                       yg::ResourceManager::Params const & rmParams,
                       shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent, ScreenBase const & screenBase);

  bool IsTiling() const;
};
