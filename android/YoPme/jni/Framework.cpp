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
    m_framework.LoadBookmarks();
  }

  Framework::~Framework()
  {
    m_framework.PrepareToShutdown();
  }

  bool Framework::ShowRect(double lat, double lon, double zoom,
                           bool needApiMark, bool needMyLoc, double myLat, double myLon)
  {
    m2::PointD point(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
    m2::PointD altPoint(MercatorBounds::LonToX(myLon), MercatorBounds::LatToY(myLat));
    if (!m_framework.IsCountryLoaded(point) && zoom > scales::GetUpperWorldScale())
      return false;

    m_framework.ShowRect(lat, lon, zoom);
    InitRenderPolicy(needApiMark, point, needMyLoc, altPoint);
    RenderMap();
    TeardownRenderPolicy();
    return true;
  }

  void Framework::InitRenderPolicy(bool needApiPin, m2::PointD const & apiPinPoint,
                                   bool needMyLoc, m2::PointD const & myLocPoint)
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
      YopmeRP * rp = new YopmeRP(rpParams);
      m_framework.SetRenderPolicy(rp);
      m_framework.InitGuiSubsystem();
      m_framework.OnSize(m_width, m_height);
      rp->DrawApiPin(needApiPin, m_framework.GtoP(apiPinPoint));
      rp->DrawMyLocation(needMyLoc, m_framework.GtoP(myLocPoint));
    }
    catch(RootException & e)
    {
      LOG(LERROR, (e.what()));
    }
  }

  void Framework::TeardownRenderPolicy()
  {
    m_framework.SetRenderPolicy(0);
  }

  void Framework::RenderMap()
  {
    Drawer * drawer = m_framework.GetRenderPolicy()->GetDrawer().get();
    shared_ptr<PaintEvent> pe(new PaintEvent(drawer));

    m_framework.BeginPaint(pe);
    m_framework.DoPaint(pe);
    m_framework.EndPaint(pe);
  }
} //yopme
