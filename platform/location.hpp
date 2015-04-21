#pragma once

#include "base/base.hpp"

#include "routing/turns.hpp"

#include "std/cmath.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace location
{
  /// @note Do not change values of this constants.
  enum TLocationError
  {
    ENoError = 0,
    ENotSupported,
    EDenied,
    EGPSIsOff
  };

  enum TLocationSource
  {
    EAppleNative,
    EWindowsNative,
    EAndroidNative,
    EGoogle,
    ETizen,
    EPredictor
  };

  /// Our structure ALWAYS has valid lat, lon and horizontal accuracy.
  /// We filter out location events without lat/lon/acc in native code as we don't need them.
  class GpsInfo
  {
  public:
    GpsInfo()
      : m_horizontalAccuracy(100.0),  // use as a default accuracy
        m_altitude(0.0), m_verticalAccuracy(-1.0), m_bearing(-1.0), m_speed(-1.0)
    {
    }

    TLocationSource m_source;
    double m_timestamp;           //!< seconds from 1st Jan 1970
    double m_latitude;            //!< degrees
    double m_longitude;           //!< degrees
    double m_horizontalAccuracy;  //!< metres
    double m_altitude;            //!< metres
    double m_verticalAccuracy;    //!< metres
    double m_bearing;             //!< positive degrees from the true North
    double m_speed;               //!< metres per second

    //bool HasAltitude() const { return m_verticalAccuracy >= 0.0; }
    bool HasBearing() const  { return m_bearing >= 0.0; }
    bool HasSpeed() const    { return m_speed >= 0.0; }
  };

  class CompassInfo
  {
  public:
    //double m_timestamp;           //!< seconds from 1st Jan 1970
    //double m_magneticHeading;     //!< positive radians from the magnetic North
    //double m_trueHeading;         //!< positive radians from the true North
    //double m_accuracy;            //!< offset from the magnetic to the true North in radians
    double m_bearing;             //!< positive radians from the true North
  };

  static inline bool IsLatValid(double lat)
  {
    return lat != 0. && lat < 90. && lat > -90.;
  }
  static inline bool IsLonValid(double lon)
  {
    return lon != 0. && lon < 180. && lon > -180.;
  }


  // Convert angle (in degrees counterclockwise from X) to bearing ([0, 360) clockwise from the north)
  static inline double AngleToBearing(double a)
  {
    double reverseAng = fmod(-a + 90, 360.);
    if (reverseAng < 0)
      reverseAng += 360.;
    return reverseAng;
  }

  // Convert bearing (in degrees clockwise from the north) to angle ([0, 360) counterclockwise from X)
  static inline double BearingToAngle(double a)
  {
    return AngleToBearing(a);
  }

  class FollowingInfo
  {
  public:
    /// @name Formatted covered distance with measurement units suffix.
    //@{
    string m_distToTarget;
    string m_targetUnitsSuffix;
    //@}

    /// @name Formated distance to next turn with measurement unit suffix
    //@{
    string m_distToTurn;
    string m_turnUnitsSuffix;
    routing::turns::TurnDirection m_turn;
    uint32_t m_exitNum;
    //@}
    int m_time;
    // m_lanes contains lane information on the edge before the turn.
    // Template parameter int is used for passing the information to Android and iOS.
    vector<vector<int8_t>> m_lanes;
    // The next street name
    string m_targetName;

    bool IsValid() const { return !m_distToTarget.empty(); }
  };

} // namespace location
