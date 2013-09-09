#include "Framework.hpp"

#include "Stubs.hpp"

#include "../../../base/logging.hpp"
#include "../../../platform/platform.hpp"
#include "../../../geometry/any_rect2d.hpp"
#include "../../../map/events.hpp"
#include "../../../map/navigator.hpp"
#include "../../../map/yopme_render_policy.hpp"

#include <android/log.h>

namespace yopme
{
  Framework::Framework(int width, int height)
  {
    m_timer.reset(new EmptyVideoTimer());
    shared_ptr<RenderContext> primaryRC(new RenderContext());
    graphics::ResourceManager::Params rmParams;
    rmParams.m_rtFormat = graphics::Data8Bpp;
    rmParams.m_texFormat = graphics::Data4Bpp;
    rmParams.m_texRtFormat = graphics::Data4Bpp;
    rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

    RenderPolicy::Params rpParams;

    rpParams.m_videoTimer = m_timer.get();
    rpParams.m_useDefaultFB = true;
    rpParams.m_rmParams = rmParams;
    rpParams.m_primaryRC = primaryRC;
    rpParams.m_density = graphics::EDensityXHDPI;
    rpParams.m_skinName = "basic.skn";
    rpParams.m_screenWidth = width;
    rpParams.m_screenHeight = height;

    try
    {
      RenderPolicy * policy = new ::YopmeRP(rpParams);
      m_framework.SetRenderPolicy(policy);
      // TODO move this in some method like ExternalStorageConnected
      m_framework.AddLocalMaps();
    }
    catch(RootException & e)
    {
      LOG(LERROR, (e.what()));
    }

    m_framework.OnSize(width, height);
  }

  Framework::~Framework()
  {
    m_framework.PrepareToShutdown();
  }

  void Framework::ConfigureNavigator(double lat, double lon, double zoom)
  {
    Navigator & navigator = m_framework.GetNavigator();
    ScalesProcessor const & scales = navigator.GetScaleProcessor();
    m2::RectD rect = scales.GetRectForDrawScale(zoom, m2::PointD(lon, lat));

    m2::PointD leftBottom = rect.LeftBottom();
    m2::PointD rightTop = rect.RightTop();
    m2::RectD pixelRect = m2::RectD(MercatorBounds::LonToX(leftBottom.x),
                                    MercatorBounds::LatToY(leftBottom.y),
                                    MercatorBounds::LonToX(rightTop.x),
                                    MercatorBounds::LatToY(rightTop.y));

    navigator.SetFromRect(m2::AnyRectD(pixelRect));
  }

  void Framework::RenderMap()
  {
    shared_ptr<PaintEvent> pe(new PaintEvent(m_framework.GetRenderPolicy()->GetDrawer().get()));

    m_framework.BeginPaint(pe);
    m_framework.DoPaint(pe);
    m_framework.EndPaint(pe);
  }
} //yopme
