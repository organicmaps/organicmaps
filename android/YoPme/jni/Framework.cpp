#include "Framework.hpp"

#include "../../../map/events.hpp"
#include "../../../map/navigator.hpp"
#include "../../../map/yopme_render_policy.hpp"

#include "../../../geometry/mercator.hpp"

#include "../../../platform/platform.hpp"

#include "../../../geometry/any_rect2d.hpp"
#include "../../../geometry/distance_on_sphere.hpp"

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
    m_framework.OnSize(width, height);

    OnMapFileUpdate();
    OnKmlFileUpdate();
  }

  Framework::~Framework()
  {
    m_framework.PrepareToShutdown();
  }

  bool Framework::ShowMap(double  vpLat,    double vpLon,  double zoom,
                          bool hasPoi,      double poiLat, double poiLon,
                          bool hasLocation, double myLat,  double myLon)
  {
    m2::PointD const viewPortCenter(MercatorBounds::FromLatLon(vpLat, vpLon));

    if (!m_framework.IsCountryLoaded(viewPortCenter) && (zoom > scales::GetUpperWorldScale()))
      return false;

    m_framework.ShowRect(vpLat, vpLon, zoom);
    m2::PointD const poi(MercatorBounds::FromLatLon(poiLat, poiLon));
    m2::PointD const myLocation(MercatorBounds::FromLatLon(myLat, myLon));
    ShowRect(hasPoi, poi, hasLocation, myLocation);

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

    YopmeRP * rp = new YopmeRP(rpParams);
    m_framework.SetRenderPolicy(rp);
    m_framework.InitGuiSubsystem();

    rp->SetDrawingApiPin(needApiPin, m_framework.GtoP(apiPinPoint));
    rp->SetDrawingMyLocation(needMyLoc, m_framework.GtoP(myLocPoint));
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

  bool Framework::AreLocationsFarEnough(double lat1, double lon1, double lat2, double lon2) const
  {
    double const sqPixLength =
        m_framework.GtoP(MercatorBounds::FromLatLon(lat1, lon1))
        .SquareLength(m_framework.GtoP(MercatorBounds::FromLatLon(lat2, lon2)));

    // Pixel radius of location mark is 10 pixels.
    return (sqPixLength > 100 && ms::DistanceOnEarth(lat1, lon1, lat2, lon2) > 5.0);
  }

} //yopme
