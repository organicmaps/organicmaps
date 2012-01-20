#include "location_state.hpp"
#include "drawer_yg.hpp"

#include "../platform/location.hpp"
#include "../platform/platform.hpp"

#include "../indexer/mercator.hpp"

namespace location
{

  State::State() : m_flags(ENone)
  {
  }

  void State::UpdateGps(GpsInfo const & info)
  {
    m_flags |= EGps;
    m_positionMercator = m2::PointD(MercatorBounds::LonToX(info.m_longitude),
                                    MercatorBounds::LatToY(info.m_latitude));
    m2::RectD const errorRectXY =
        MercatorBounds::MetresToXY(info.m_longitude, info.m_latitude,
                                   info.m_horizontalAccuracy);
    m_errorRadiusMercator = sqrt(my::sq(errorRectXY.SizeX()) + my::sq(errorRectXY.SizeY())) / 4;
  }

  void State::UpdateCompass(CompassInfo const & info)
  {
    m_flags |= ECompass;

    m_headingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading)
        / 180 * math::pi
        - math::pi / 2;  // 0 angle is for North ("up"), but in our coordinates it's to the right.
    m_headingAccuracyRad = info.m_accuracy / 180 * math::pi;
  }

  void State::DrawMyPosition(DrawerYG & drawer, ScreenBase const & screen)
  {
    double pxErrorRadius;
    m2::PointD pxPosition;
    m2::PointD pxShift(screen.PixelRect().minX(), screen.PixelRect().minY());

    if ((m_flags & State::EGps) || (m_flags & State::ECompass))
    {
      pxPosition = screen.GtoP(Position());
      pxErrorRadius = pxPosition.Length(screen.GtoP(Position() + m2::PointD(ErrorRadius(), 0)));

      pxPosition -= pxShift;

      if (m_flags & State::EGps)
      {
        // my position symbol
        drawer.drawSymbol(pxPosition, "current-position", yg::EPosCenter, yg::maxDepth);
        // my position circle
        drawer.screen()->fillSector(pxPosition, 0, math::pi * 2, pxErrorRadius,
                                      yg::Color(0, 0, 255, 32),
                                      yg::maxDepth - 3);
        // display compass only if position is available

        double orientationRadius = max(pxErrorRadius, 30 * GetPlatform().VisualScale());

        if (m_flags & State::ECompass)
        {
          drawer.screen()->drawSector(pxPosition,
                m_headingRad - m_headingAccuracyRad,
                m_headingRad + m_headingAccuracyRad,
                orientationRadius,
                yg::Color(255, 255, 255, 192),
                yg::maxDepth);
          drawer.screen()->fillSector(pxPosition,
                m_headingRad - m_headingAccuracyRad,
                m_headingRad + m_headingAccuracyRad,
                orientationRadius,
                yg::Color(255, 255, 255, 96),
                yg::maxDepth - 1);
        }
      }
    }
  }
}
