#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

class DrawerYG;
class Navigator;

namespace location
{
  class GpsInfo;
  class CompassInfo;

  class State
  {
    double m_errorRadiusMercator;
    m2::PointD m_positionMercator;

    double m_headingRad;
    /// Angle to the left and to the right from the North
    double m_headingHalfSectorRad;

    int m_flags;      ///< stores flags from SymbolType

  public:
    enum SymbolType
    {
      ENone = 0x0,
      EGps = 0x1,
      ECompass = 0x2
    };

    State();

    /// @return GPS center point in mercator
    m2::PointD Position() const { return m_positionMercator; }

    inline bool IsValidPosition() const { return ((m_flags & EGps) != 0); }
    inline void TurnOff() { m_flags = ENone; }

    /// @param[in] rect Bound rect for circle with position center and error radius.
    void UpdateGps(m2::RectD const & rect);
    void UpdateCompass(CompassInfo const & info);

    void DrawMyPosition(DrawerYG & drawer, Navigator const & nav);

    operator int() const
    {
      return m_flags;
    }
  };
}
