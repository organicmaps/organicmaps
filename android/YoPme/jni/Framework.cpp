#include "Framework.hpp"

#include "../../../map/events.hpp"
#include "../../../map/navigator.hpp"
#include "../../../map/yopme_render_policy.hpp"
#include "../../../platform/platform.hpp"
#include "../../../geometry/any_rect2d.hpp"
#include "../../../graphics/opengl/gl_render_context.hpp"
#include "../../../base/logging.hpp"

#include <android/log.h>

namespace
{
  class RenderContext: public graphics::gl::RenderContext
  {
  public:
    virtual void makeCurrent() {}
    virtual graphics::RenderContext * createShared() { return this; }
  };
}

namespace yopme
{
  Framework::Framework(int width, int height)
    : m_width(width)
    , m_height(height)
  {
    // TODO move this in some method like ExternalStorageConnected
    m_framework.AddLocalMaps();
  }

  Framework::~Framework()
  {
    m_framework.PrepareToShutdown();
  }

  bool Framework::ShowRect(double lat, double lon, double zoom, bool needApiMark)
  {
    m2::PointD point(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
    if (!m_framework.IsCountryLoaded(point) && zoom > scales::GetUpperWorldScale())
      return false;

    m_framework.ShowRect(lat, lon, zoom);
    InitRenderPolicy();
    RenderMap(point, needApiMark ? "api_pin" : "current-position");
    TeardownRenderPolicy();
    return true;
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

    rpParams.m_videoTimer = &m_timer;
    rpParams.m_useDefaultFB = true;
    rpParams.m_rmParams = rmParams;
    rpParams.m_primaryRC = primaryRC;
    rpParams.m_density = graphics::EDensityMDPI;
    rpParams.m_skinName = "basic.skn";
    rpParams.m_screenWidth = m_width;
    rpParams.m_screenHeight = m_height;

    try
    {
      m_framework.SetRenderPolicy(new ::YopmeRP(rpParams));
      m_framework.InitGuiSubsystem();
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

  void Framework::RenderMap(const m2::PointD & markPoint, const string & symbolName)
  {
    Drawer * drawer = m_framework.GetRenderPolicy()->GetDrawer().get();
    shared_ptr<PaintEvent> pe(new PaintEvent(drawer));

    m_framework.BeginPaint(pe);
    m_framework.DoPaint(pe);
    m_framework.GetInformationDisplay().drawPlacemark(drawer, symbolName, m_framework.GtoP(markPoint));
    m_framework.EndPaint(pe);
  }
} //yopme
