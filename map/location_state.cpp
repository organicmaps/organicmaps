#include "location_state.hpp"

#include "../platform/location.hpp"

#include "../indexer/mercator.hpp"

namespace location
{

  State::State() : m_deviceOrientation(-math::pi / 2), m_type(ENone)
  {
  }

  void State::UpdateGps(GpsInfo const & info)
  {
    if (info.m_status == EAccurateMode
        || info.m_status == ERoughMode)
    {
      m_type |= EGps;
      if (info.m_status == EAccurateMode)
        m_type |= EPreciseMode;
      else
        m_type &= !EPreciseMode;

      m_positionMercator = m2::PointD(MercatorBounds::LonToX(info.m_longitude),
                                        MercatorBounds::LatToY(info.m_latitude));
      m2::RectD const errorRectXY =
          MercatorBounds::MetresToXY(info.m_longitude, info.m_latitude,
                                     info.m_horizontalAccuracy);
      m_errorRadiusMercator = sqrt((errorRectXY.SizeX() * errorRectXY.SizeX()
                               + errorRectXY.SizeY() * errorRectXY.SizeY()) / 4);
    }
    else
    {
      m_type &= !EGps;
    }
  }

  void State::UpdateCompass(CompassInfo const & info)
  {
    m_type |= ECompass;

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
}
