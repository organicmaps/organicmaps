#include "Framework.hpp"

#include "Stubs.hpp"

#include "../../../map/events.hpp"
#include "../../../map/navigator.hpp"
#include "../../../map/yopme_render_policy.hpp"
#include "../../../platform/platform.hpp"
#include "../../../geometry/any_rect2d.hpp"
#include "../../../base/logging.hpp"

#include <android/log.h>

namespace yopme
{
  static EmptyVideoTimer s_timer;
  Framework::Framework(int width, int height)
    : m_width(width)
    , m_height(height)
  {
    LOG(LDEBUG, ("Framework Constructor"));
    // TODO move this in some method like ExternalStorageConnected
    m_framework.AddLocalMaps();
    LOG(LDEBUG, ("Local maps addeded"));
  }

  Framework::~Framework()
  {
    m_framework.PrepareToShutdown();
  }

  void Framework::ShowRect(double lat, double lon, double zoom)
  {
    m_framework.ShowRect(lat, lon, zoom);
    InitRenderPolicy();
    RenderMap();
    TeardownRenderPolicy();
  }

  void Framework::InitRenderPolicy()
  {
    shared_ptr<RenderContext> primaryRC(new RenderContext());
    graphics::ResourceManager::Params rmParams;
    rmParams.m_rtFormat = graphics::Data8Bpp;
    rmParams.m_texFormat = graphics::Data4Bpp;
    rmParams.m_texRtFormat = graphics::Data4Bpp;
    rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

    RenderPolicy::Params rpParams;

    rpParams.m_videoTimer = &s_timer;
    rpParams.m_useDefaultFB = true;
    rpParams.m_rmParams = rmParams;
    rpParams.m_primaryRC = primaryRC;
    rpParams.m_density = graphics::EDensityXHDPI;
    rpParams.m_skinName = "basic.skn";
    rpParams.m_screenWidth = m_width;
    rpParams.m_screenHeight = m_height;

    try
    {
      m_framework.SetRenderPolicy(new ::YopmeRP(rpParams));
    }
    catch(RootException & e)
    {
      LOG(LERROR, (e.what()));
    }

    m_framework.OnSize(m_width, m_height);
  }

  void Framework::TeardownRenderPolicy()
  {
    m_framework.SetRenderPolicy(0);
  }

  void Framework::RenderMap()
  {
    shared_ptr<PaintEvent> pe(new PaintEvent(m_framework.GetRenderPolicy()->GetDrawer().get()));

    m_framework.BeginPaint(pe);
    m_framework.DoPaint(pe);
    m_framework.EndPaint(pe);
  }
} //yopme
