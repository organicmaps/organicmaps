#pragma once

#include "platform/location_clock.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cmath>
#include <functional>
#include <optional>
#include <string>

namespace location
{
/// @note Do not change values of this constants.
enum TLocationError
{
  ENoError = 0,
  ENotSupported,
  EDenied,
  EGPSIsOff,
  ETimeout,  // Only used on Qt https://doc.qt.io/qt-6/qgeopositioninfosource.html#Error-enum
  EUnknown
};

inline std::string DebugPrint(TLocationError error)
{
  switch (error)
  {
  case ENoError: return "ENoError";
  case ENotSupported: return "ENotSupported";
  case EDenied: return "EDenied";
  case EGPSIsOff: return "EGPSIsOff";
  case ETimeout: return "ETimeout";
  case EUnknown: return "EUnknown";
  }
  UNREACHABLE();
}

enum TLocationSource
{
  EUndefined,
  EAppleNative,
  EWindowsNative,
  EAndroidNative,
  EGoogle,
  ETizen,  // Deprecated but left here for backward compatibility.
  EGeoClue2,
  EPredictor,
  EUser
};

/// Our structure ALWAYS has valid lat, lon and horizontal accuracy.
/// We filter out location events without lat/lon/acc in native code as we don't need them.
class GpsInfo
{
public:
  struct Timing
  {
    Clock::time_point m_observedAt;
    Clock::time_point m_liveUntil;

    bool operator==(Timing const &) const = default;
  };

  TLocationSource m_source = EUndefined;
  double m_timestamp = 0.0;             //!< seconds from 1st Jan 1970
  double m_latitude = 0.0;              //!< degrees
  double m_longitude = 0.0;             //!< degrees
  double m_horizontalAccuracy = 100.0;  //!< metres
  double m_altitude = 0.0;              //!< metres
  double m_verticalAccuracy = -1.0;     //!< metres
  double m_bearing = -1.0;              //!< positive degrees from the true North
  double m_speed = -1.0;                //!< metres per second
  // Transient observation metadata. Predictions retain the observation's age.
  // Producers without a compatible clock leave this unset; recording keeps UTC.
  std::optional<Timing> m_timing = std::nullopt;

  bool IsValid() const { return m_source != EUndefined; }
  bool IsFresh(Clock::time_point now = Clock::now()) const
  {
    return !m_timing || (m_timing->m_observedAt <= now && now < m_timing->m_liveUntil);
  }
  bool HasSameClock(GpsInfo const & other) const { return m_timing.has_value() == other.m_timing.has_value(); }
  bool HasBearing() const { return m_bearing >= 0.0; }
  bool HasSpeed() const { return m_speed >= 0.0; }
  bool HasAltitude() const { return m_verticalAccuracy >= 0.0; }
  ms::LatLon GetLatLon() const { return {m_latitude, m_longitude}; }
};

struct CompassInfo
{
  double m_bearing = 0.0;  //!< positive radians from the true North
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

class RouteMatchingInfo
{
  m2::PointD m_matchedPosition;
  size_t m_indexInRoute;
  bool m_isPositionMatched;
  bool m_hasDistanceFromBegin;
  double m_distanceFromBegin;

public:
  RouteMatchingInfo()
    : m_matchedPosition(0., 0.)
    , m_indexInRoute(0)
    , m_isPositionMatched(false)
    , m_hasDistanceFromBegin(false)
    , m_distanceFromBegin(0.0)
  {}

  void Set(m2::PointD const & matchedPosition, size_t indexInRoute, double distanceFromBegin)
  {
    m_matchedPosition = matchedPosition;
    m_indexInRoute = indexInRoute;
    m_isPositionMatched = true;

    m_distanceFromBegin = distanceFromBegin;
    m_hasDistanceFromBegin = true;
  }

  void Reset() { m_isPositionMatched = false; }
  bool IsMatched() const { return m_isPositionMatched; }
  size_t GetIndexInRoute() const { return m_indexInRoute; }
  m2::PointD GetPosition() const { return m_matchedPosition; }
  bool HasDistanceFromBegin() const { return m_hasDistanceFromBegin; }
  double GetDistanceFromBegin() const { return m_distanceFromBegin; }
};

enum EMyPositionMode
{
  PendingPosition = 0,
  NotFollowNoPosition,
  NotFollow,
  Follow,
  FollowAndRotate
};

using TMyPositionModeChanged = std::function<void(location::EMyPositionMode, bool)>;

}  // namespace location
