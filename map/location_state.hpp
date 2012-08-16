#pragma once

#include "../platform/location.hpp"

#include "../geometry/point2d.hpp"

#include "../std/shared_ptr.hpp"

class DrawerYG;
class Framework;
class RotateScreenTask;

namespace location
{
  class GpsInfo;
  class CompassInfo;

  enum ELocationProcessMode
  {
    ELocationDoNothing,
    ELocationCenterAndScale,
    ELocationCenterOnly,
    ELocationSkipCentering
  };

  enum ECompassProcessMode
  {
    ECompassDoNothing,
    ECompassFollow
  };

  // Class, that handles position and compass updates,
  // centers, scales and rotates map according to this updates
  // and draws location and compass marks.
  class State
  {
  private:
    double m_errorRadius; //< error radius in mercator
    m2::PointD m_position; //< position in mercator

    double m_headingRad; //< direction in radians
    double m_headingHalfErrorRad; //< direction error in radians

    bool m_hasPosition;
    bool m_hasCompass;

    ELocationProcessMode m_locationProcessMode;
    ECompassProcessMode m_compassProcessMode;

    shared_ptr<RotateScreenTask> m_rotateScreenTask;

    Framework * m_fw;

    void FollowCompass();

  public:

    State(Framework * framework);

    /// @return GPS center point in mercator
    m2::PointD const & Position() const { return m_position; }

    bool HasPosition() const;
    bool HasCompass() const;

    ELocationProcessMode LocationProcessMode() const;
    void SetLocationProcessMode(ELocationProcessMode mode);

    ECompassProcessMode CompassProcessMode() const;
    void SetCompassProcessMode(ECompassProcessMode mode);

    void TurnOff();

    void Draw(DrawerYG & drawer);
    void StopAnimation();

    /// @name GPS location updates routine.
    //@{
    void SkipLocationCentering();
    void OnLocationStatusChanged(location::TLocationStatus newStatus);
    void OnGpsUpdate(location::GpsInfo const & info);
    void OnCompassUpdate(location::CompassInfo const & info);
    //@}
  };
}
