#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/function.hpp"

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
    ETizen
  };

  /// Our structure ALWAYS has valid lat, lon and horizontal accuracy.
  /// We filter out location events without lat/lon/acc in native code as we don't need them.
  class GpsInfo
  {
  public:
    TLocationSource m_source;
    double m_timestamp;           //!< seconds from 1st Jan 1970
    double m_latitude;            //!< degrees
    double m_longitude;           //!< degrees
    double m_horizontalAccuracy;  //!< metres
    double m_altitude;            //!< metres
    double m_verticalAccuracy;    //!< metres
    double m_course;              //!< positive degrees from the true North
    double m_speed;               //!< metres per second

    bool HasAltitude() const { return m_verticalAccuracy >= 0.; }
    bool HasBearing() const  { return m_course >= 0.; }
    bool HasSpeed() const    { return m_speed >= 0.; }
  };

  class CompassInfo
  {
  public:
    double m_timestamp;           //!< how many seconds ago the heading was retrieved
    double m_magneticHeading;     //!< positive radians from the magnetic North
    double m_trueHeading;         //!< positive radians from the true North
    double m_accuracy;            //!< offset from the magnetic to the true North in radians
  };

  static inline bool IsLatValid(double lat)
  {
    return lat != 0. && lat < 90. && lat > -90.;
  }
  static inline bool IsLonValid(double lon)
  {
    return lon != 0. && lon < 180. && lon > -180.;
  }

} // namespace location
