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

  bool Framework::ShowMyPosition(double lat, double lon, double zoom)
  {
    m2::PointD position(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
    if (!m_framework.IsCountryLoaded(position) && zoom > scales::GetUpperWorldScale())
      return false;

    m_framework.ShowRect(lat, lon, zoom);
    ShowRect(false, m2::PointD(), true, position);
    return true;
  }

  bool Framework::ShowPoi(double lat, double lon, bool needMyLoc, double myLat, double myLoc, double zoom)
  {
    m2::PointD poi(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
    if (!m_framework.IsCountryLoaded(poi) && zoom > scales::GetUpperWorldScale())
      return false;

    m_framework.ShowRect(lat, lon, zoom);
    m2::PointD position(MercatorBounds::LonToX(myLoc), MercatorBounds::LatToY(myLat));
    ShowRect(true, poi, needMyLoc, position);
    return true;
  }

  void Framework::ShowRect(bool needApiPin, m2::PointD const & apiPinPoint,
                           bool needMyLoc, m2::PointD const & myLocPoint)
  {
    InitRenderPolicy(needApiPin, apiPinPoint, needMyLoc, myLocPoint);
    RenderMap();
    TeardownRenderPolicy();
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
      rp->SetDrawingApiPin(needApiPin, m_framework.GtoP(apiPinPoint));
      rp->SetDrawingMyLocation(needMyLoc, m_framework.GtoP(myLocPoint));
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

  void Framework::OnKmlFileUpdate()
  {
    m_framework.LoadBookmarks();
  }

  void Framework::OnMapFileUpdate()
  {
    m_framework.RemoveLocalMaps();
    m_framework.AddLocalMaps();
  }
} //yopme
