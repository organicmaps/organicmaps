#include "location_state.hpp"
#include "drawer_yg.hpp"
#include "navigator.hpp"

#include "../platform/location.hpp"
#include "../platform/platform.hpp"

#include "../indexer/mercator.hpp"


namespace location
{

  State::State() : m_flags(ENone)
  {
  }

  void State::UpdateGps(m2::RectD const & rect)
  {
    m_flags |= EGps;

    m_positionMercator = rect.Center();

    //m_errorRadiusMercator = sqrt(my::sq(rect.SizeX()) + my::sq(rect.SizeY())) / 2;
    m_errorRadiusMercator = rect.SizeX() / 2.0;
  }

  void State::UpdateCompass(CompassInfo const & info)
  {
    m_flags |= ECompass;

    m_headingRad = ((info.m_trueHeading >= 0.0) ? info.m_trueHeading : info.m_magneticHeading);
    // 0 angle is for North ("up"), but in our coordinates it's to the right.
    m_headingRad = m_headingRad - math::pi / 2.0;

    // Avoid situations when offset between magnetic north and true north is too small
    static double const MIN_SECTOR_RAD = math::pi / 18.0;
    m_headingHalfSectorRad = (info.m_accuracy < MIN_SECTOR_RAD ? MIN_SECTOR_RAD : info.m_accuracy);
  }

  void State::DrawMyPosition(DrawerYG & drawer, Navigator const & nav)
  {
    if ((m_flags & State::EGps) || (m_flags & State::ECompass))
    {
      m2::PointD const pxPosition = nav.GtoP(Position());
      double const pxErrorRadius = pxPosition.Length(nav.GtoP(Position() + m2::PointD(m_errorRadiusMercator, 0.0)));

      if (m_flags & State::EGps)
      {
        // my position symbol
        drawer.drawSymbol(pxPosition, "current-position", yg::EPosCenter, yg::maxDepth);

        // my position circle
        drawer.screen()->fillSector(pxPosition, 0, 2.0 * math::pi, pxErrorRadius,
                                      yg::Color(0, 0, 255, 32),
                                      yg::maxDepth - 3);

        // display compass only if position is available
        double orientationRadius = max(pxErrorRadius, 30.0 * drawer.VisualScale());

        double screenAngle = nav.Screen().GetAngle();

        if (m_flags & State::ECompass)
        {
          drawer.screen()->drawSector(pxPosition,
                screenAngle + m_headingRad - m_headingHalfSectorRad,
                screenAngle + m_headingRad + m_headingHalfSectorRad,
                orientationRadius,
                yg::Color(255, 255, 255, 192),
                yg::maxDepth);

          drawer.screen()->fillSector(pxPosition,
                screenAngle + m_headingRad - m_headingHalfSectorRad,
                screenAngle + m_headingRad + m_headingHalfSectorRad,
                orientationRadius,
                yg::Color(255, 255, 255, 96),
                yg::maxDepth - 1);
        }
      }
    }
  }
}
