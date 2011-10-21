#include "location_state.hpp"
#include "drawer_yg.hpp"

#include "../platform/location.hpp"

#include "../indexer/mercator.hpp"

namespace location
{

  State::State() : m_deviceOrientation(-math::pi / 2), m_flags(ENone)
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
    m_errorRadiusMercator = sqrt((errorRectXY.SizeX() * errorRectXY.SizeX()
                                  + errorRectXY.SizeY() * errorRectXY.SizeY()) / 4);
  }

  void State::UpdateCompass(CompassInfo const & info)
  {
    m_flags |= ECompass;

    m_headingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading)
        / 180 * math::pi;
    m_headingAccuracyRad = info.m_accuracy / 180 * math::pi;
  }

  void State::SetOrientation(EOrientation orientation)
  {
    switch (orientation)
    {
    case EOrientation0:
      m_deviceOrientation = -math::pi / 2;
      break;
    case EOrientation90:
      m_deviceOrientation = math::pi;
      break;
    case EOrientation180:
      m_deviceOrientation = math::pi / 2;
      break;
    case EOrientation270:
      m_deviceOrientation = 0;
      break;
    }
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
        if (m_flags & State::ECompass)
        {
          drawer.screen()->drawSector(pxPosition,
                m_deviceOrientation + m_headingRad - m_headingAccuracyRad,
                m_deviceOrientation + m_headingRad + m_headingAccuracyRad,
                pxErrorRadius,
                yg::Color(255, 255, 255, 192),
                yg::maxDepth);
          drawer.screen()->fillSector(pxPosition,
                m_deviceOrientation + m_headingRad - m_headingAccuracyRad,
                m_deviceOrientation + m_headingRad + m_headingAccuracyRad,
                pxErrorRadius,
                yg::Color(255, 255, 255, 96),
                yg::maxDepth - 1);
        }
      }
    }
  }
}
