#pragma once

#include "geometry/point2d.hpp"

#include "base/base.hpp"

#include "geometry/latlon.hpp"

#include "routing/turns.hpp"
#include "routing/turns_sound_settings.hpp"

#include "std/cmath.hpp"
#include "std/function.hpp"
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
    EUndefined,
    EAppleNative,
    EWindowsNative,
    EAndroidNative,
    EGoogle,
    ETizen,
    EPredictor,
    EUser
  };

  /// Our structure ALWAYS has valid lat, lon and horizontal accuracy.
  /// We filter out location events without lat/lon/acc in native code as we don't need them.
  class GpsInfo
  {
  public:
    TLocationSource m_source = EUndefined;
    /// @TODO(bykoianko) |m_timestamp| is calculated based on platform methods which don't
    /// guarantee that |m_timestamp| is monotonic. |m_monotonicTimeMs| should be added to
    /// class |GpsInfo|. This time should be calculated based on Location::getElapsedRealtimeNanos()
    /// method in case of Android. How to calculate such time in case of iOS should be
    /// investigated.
    /// \note For most cases |m_timestamp| is monotonic.
    double m_timestamp = 0.0;             //!< seconds from 1st Jan 1970
    double m_latitude = 0.0;              //!< degrees
    double m_longitude = 0.0;             //!< degrees
    double m_horizontalAccuracy = 100.0;  //!< metres
    double m_altitude = 0.0;              //!< metres
    double m_verticalAccuracy = -1.0;     //!< metres
    double m_bearing = -1.0;              //!< positive degrees from the true North
    double m_speed = -1.0;                //!< metres per second

    bool IsValid() const { return m_source != EUndefined; }
    bool HasBearing() const { return m_bearing >= 0.0; }
    bool HasSpeed() const { return m_speed >= 0.0; }
    bool HasVerticalAccuracy() const { return m_verticalAccuracy >= 0.0; }
  };

  /// GpsTrackInfo struct describes a point for GPS tracking
  /// It is similar to the GpsInfo but contains only needed fields.
  struct GpsTrackInfo
  {
    double m_timestamp; //!< seconds from 1st Jan 1970
    double m_latitude;  //!< degrees
    double m_longitude; //!< degrees
    double m_speed;     //!< meters per second

    GpsTrackInfo() = default;
    GpsTrackInfo(GpsTrackInfo const &) = default;
    GpsTrackInfo & operator=(GpsTrackInfo const &) = default;
    GpsTrackInfo(GpsInfo const & info)
      : m_timestamp(info.m_timestamp)
      , m_latitude(info.m_latitude)
      , m_longitude(info.m_longitude)
      , m_speed(info.m_speed)
    {}
    GpsTrackInfo & operator=(GpsInfo const & info)
    {
      return operator=(GpsTrackInfo(info));
    }
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
    FollowingInfo()
        : m_turn(routing::turns::CarDirection::None),
          m_nextTurn(routing::turns::CarDirection::None),
          m_exitNum(0),
          m_time(0),
          m_completionPercent(0),
          m_pedestrianTurn(routing::turns::PedestrianDirection::None),
          m_pedestrianDirectionPos(0., 0.)
    {
    }

    // SingleLaneInfoClient is used for passing information about a lane to client platforms such as
    // Android, iOS and so on.
    struct SingleLaneInfoClient
    {
      vector<int8_t> m_lane;  // Possible directions for the lane.
      bool m_isRecommended;   // m_isRecommended is true if the lane is recommended for a user.

      SingleLaneInfoClient(routing::turns::SingleLaneInfo const & singleLaneInfo)
          : m_isRecommended(singleLaneInfo.m_isRecommended)
      {
        routing::turns::TSingleLane const & lane = singleLaneInfo.m_lane;
        m_lane.resize(lane.size());
        transform(lane.cbegin(), lane.cend(), m_lane.begin(), [](routing::turns::LaneWay l)
        {
          return static_cast<int8_t>(l);
        });
      }
    };

    /// @name Formatted covered distance with measurement units suffix.
    //@{
    string m_distToTarget;
    string m_targetUnitsSuffix;
    //@}

    /// @name Formated distance to next turn with measurement unit suffix
    //@{
    string m_distToTurn;
    string m_turnUnitsSuffix;
    routing::turns::CarDirection m_turn;
    /// Turn after m_turn. Returns NoTurn if there is no turns after.
    routing::turns::CarDirection m_nextTurn;
    uint32_t m_exitNum;
    //@}
    int m_time;
    // m_lanes contains lane information on the edge before the turn.
    vector<SingleLaneInfoClient> m_lanes;
    // m_turnNotifications contains information about the next turn notifications.
    // If there is nothing to pronounce m_turnNotifications is empty.
    // If there is something to pronounce the size of m_turnNotifications may be one or even more
    // depends on the number of notifications to prononce.
    vector<string> m_turnNotifications;
    // Current street name.
    string m_sourceName;
    // The next street name.
    string m_targetName;
    // Street name to display. May be empty.
    string m_displayedStreetName;

    // Percentage of the route completion.
    double m_completionPercent;

    /// @name Pedestrian direction information
    //@{
    routing::turns::PedestrianDirection m_pedestrianTurn;
    ms::LatLon m_pedestrianDirectionPos;
    //@}

    bool IsValid() const { return !m_distToTarget.empty(); }
  };

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

  using TMyPositionModeChanged = function<void (location::EMyPositionMode, bool)>;

} // namespace location
