#pragma once

#include "base/base.hpp"

#include "geometry/latlon.hpp"

#include "routing/turns.hpp"
#include "routing/turns_sound_settings.hpp"

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
    FollowingInfo()
        : m_turn(routing::turns::TurnDirection::NoTurn),
          m_exitNum(0),
          m_time(0),
          m_completionPercent(0),
          m_speedWarningSignal(false),
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
    routing::turns::TurnDirection m_turn;
    /// Turn after m_turn. Returns NoTurn if there is no turns after.
    routing::turns::TurnDirection m_nextTurn;
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

    // Percentage of the route completion.
    double m_completionPercent;

    // Speed cam warning signal.
    bool m_speedWarningSignal;

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

  public:
    RouteMatchingInfo() : m_matchedPosition(0., 0.), m_indexInRoute(0), m_isPositionMatched(false) {}
    void Set(m2::PointD const & matchedPosition, size_t indexInRoute)
    {
      m_matchedPosition = matchedPosition;
      m_indexInRoute = indexInRoute;
      m_isPositionMatched = true;
    }
    void Reset() { m_isPositionMatched = false; }
    bool IsMatched() const { return m_isPositionMatched; }
    size_t GetIndexInRoute() const { return m_indexInRoute; }
    m2::PointD GetPosition() const { return m_matchedPosition; }
  };
} // namespace location
