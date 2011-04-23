#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/screenbase.hpp"

namespace location
{
  class GpsInfo;
  class CompassInfo;

  class State
  {
    double m_errorRadiusMercator;
    m2::PointD m_positionMercator;

    double m_deviceOrientation;
    double m_headingRad;
    double m_headingAccuracyRad;

  public:
    enum SymbolType
    {
      ENone = 0x0,
      EGps = 0x1,
      EPreciseMode = 0x2,
      ECompass = 0x4,
    };

    State();

    /// @return GPS error radius in mercator
    double ErrorRadius() const { return m_errorRadiusMercator; }
    /// @return GPS center point in mercator
    m2::PointD Position() const { return m_positionMercator; }
    /// takes into account device's orientation
    /// @return angle in radians
    double Heading() const { return m_deviceOrientation + m_headingRad; }
    /// @return angle in radians
    double HeadingAccuracy() const { return m_headingAccuracyRad; }

    void TurnOff() { m_type = ENone; }
    void UpdateGps(GpsInfo const & info);
    void UpdateCompass(CompassInfo const & info);
    void SetOrientation(EOrientation orientation);

    operator int() const
    {
      return m_type;
    }

  private:
    /// stores flags from SymbolType
    int m_type;
  };
}
