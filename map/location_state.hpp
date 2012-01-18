#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/screenbase.hpp"

class DrawerYG;

namespace location
{
  class GpsInfo;
  class CompassInfo;

  class State
  {
    double m_errorRadiusMercator;
    m2::PointD m_positionMercator;

    double m_headingRad;
    double m_headingAccuracyRad;

  public:
    enum SymbolType
    {
      ENone = 0x0,
      EGps = 0x1,
      ECompass = 0x2
    };

    State();

    /// @return GPS error radius in mercator
    double ErrorRadius() const { return m_errorRadiusMercator; }
    /// @return GPS center point in mercator
    m2::PointD Position() const { return m_positionMercator; }

    bool IsValidPosition() const { return ((m_flags & EGps) != 0); }

    void TurnOff() { m_flags = ENone; }
    void UpdateGps(GpsInfo const & info);
    void UpdateCompass(CompassInfo const & info);

    void DrawMyPosition(DrawerYG & drawer, ScreenBase const & screen);

    operator int() const
    {
      return m_flags;
    }

  private:
    /// stores flags from SymbolType
    int m_flags;
  };
}
