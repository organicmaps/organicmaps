#pragma once

#include "routing/route.hpp"
#include "routing/turns.hpp"

#include "platform/measurement_utils.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace routing
{
namespace turns
{
namespace sound
{
/// \brief The Settings struct is a structure for gathering information about turn sound
/// notifications settings.
/// Part distance parameters shall be set in m_lengthUnits. (Meters of feet for the time being.)
/// Another part in meters. See the suffix to understand which units are used.
class Settings
{
  friend void UnitTest_TurnNotificationSettingsMetersTest();
  friend void UnitTest_TurnNotificationSettingsFeetTest();
  friend void UnitTest_TurnNotificationSettingsNotValidTest();

  uint32_t m_timeSeconds;
  uint32_t m_minDistanceUnits;
  uint32_t m_maxDistanceUnits;

  /// To inform an end user about the next turn with the help of an voice information message
  /// an operation system needs:
  /// - to launch TTS subsystem;
  /// - to pronounce the message.
  /// So to inform the user in time it's necessary to start
  /// m_startBeforeSeconds before the time. It is used in the following way:
  /// we start playing voice notice in m_startBeforeSeconds * TurnsSound::m_speedMetersPerSecond
  /// meters before the turn (for the second voice notification).
  /// When m_startBeforeSeconds * TurnsSound::m_speedMetersPerSecond is too small or too large
  /// we use m_{min|max}StartBeforeMeters to clamp the value.
  uint32_t m_startBeforeSecondsVehicle;
  uint32_t m_minStartBeforeMetersVehicle;
  uint32_t m_maxStartBeforeMetersVehicle;

  uint32_t m_startBeforeSecondsPedestrian = 0;
  uint32_t m_minStartBeforeMetersPedestrian = 0;
  uint32_t m_maxStartBeforeMetersPedestrian = 0;

  /// m_minDistToSayNotificationMeters is minimum distance between two turns
  /// when pronouncing the first notification about the second turn makes sense.
  uint32_t m_minDistToSayNotificationMeters;

  /// \brief m_distancesToPronounce is a list of distances in m_lengthUnits
  ///  which are ready to be pronounced.
  std::vector<uint32_t> m_soundedDistancesUnits;
  measurement_utils::Units m_lengthUnits;

public:
  // This constructor is for testing only.
  // TODO: Do not compile it for production. Either use a static method or derive it in tests.
  Settings(uint32_t notificationTimeSeconds, uint32_t minNotificationDistanceUnits,
           uint32_t maxNotificationDistanceUnits, uint32_t startBeforeSeconds, uint32_t minStartBeforeMeters,
           uint32_t maxStartBeforeMeters, uint32_t minDistToSayNotificationMeters,
           std::vector<uint32_t> const & soundedDistancesUnits, measurement_utils::Units lengthUnits)
    : m_timeSeconds(notificationTimeSeconds)
    , m_minDistanceUnits(minNotificationDistanceUnits)
    , m_maxDistanceUnits(maxNotificationDistanceUnits)
    , m_startBeforeSecondsVehicle(startBeforeSeconds)
    , m_minStartBeforeMetersVehicle(minStartBeforeMeters)
    , m_maxStartBeforeMetersVehicle(maxStartBeforeMeters)
    , m_minDistToSayNotificationMeters(minDistToSayNotificationMeters)
    , m_soundedDistancesUnits(soundedDistancesUnits)
    , m_lengthUnits(lengthUnits)
  {
    ASSERT(!m_soundedDistancesUnits.empty(), ());
  }

  Settings(uint32_t startBeforeSecondsVehicle, uint32_t minStartBeforeMetersVehicle,
           uint32_t maxStartBeforeMetersVehicle, uint32_t minDistToSayNotificationMeters,
           uint32_t startBeforeSecondsPedestrian, uint32_t minStartBeforeMetersPedestrian,
           uint32_t maxStartBeforeMetersPedestrian)
    : m_timeSeconds(0)
    , m_minDistanceUnits(0)
    , m_maxDistanceUnits(0)
    , m_startBeforeSecondsVehicle(startBeforeSecondsVehicle)
    , m_minStartBeforeMetersVehicle(minStartBeforeMetersVehicle)
    , m_maxStartBeforeMetersVehicle(maxStartBeforeMetersVehicle)
    , m_startBeforeSecondsPedestrian(startBeforeSecondsPedestrian)
    , m_minStartBeforeMetersPedestrian(minStartBeforeMetersPedestrian)
    , m_maxStartBeforeMetersPedestrian(maxStartBeforeMetersPedestrian)
    , m_minDistToSayNotificationMeters(minDistToSayNotificationMeters)
    , m_lengthUnits(measurement_utils::Units::Metric)
  {}

  void SetState(uint32_t notificationTimeSeconds, uint32_t minNotificationDistanceUnits,
                uint32_t maxNotificationDistanceUnits, std::vector<uint32_t> const & soundedDistancesUnits,
                measurement_utils::Units lengthUnits);

  /// \brief IsValid checks if Settings data is consistent.
  /// \warning The complexity is up to linear in size of m_soundedDistancesUnits.
  /// \note For any instance created by default constructor IsValid() returns false.
  bool IsValid() const;

  /// \brief computes the distance an end user shall be informed about the future turn.
  /// \param speedMetersPerSecond is a speed. For example it could be a current speed of an end
  /// user.
  /// \return distance in meters.
  uint32_t ComputeTurnDistanceM(double speedMetersPerSecond) const;

  /// \brief computes the distance which will be passed at the |speedMetersPerSecond|
  /// while pronouncing turn sound notification.
  uint32_t ComputeDistToPronounceDistM(double speedMetersPerSecond, bool pedestrian = false) const;

  /// @return true if distToTurnMeters is too short to start pronouncing first turn notification.
  bool TooCloseForFisrtNotification(double distToTurnMeters) const;

  /// \brief RoundByPresetSoundedDistancesUnits rounds off its parameter by
  /// m_soundedDistancesUnits.
  /// \param turnNotificationDistance is a distance in m_lengthUnits.
  /// \return the distance which will be used (will be pronounced) in the next turn sound
  /// notification in m_lengthUnits units. (Meters of feet for the time being.)
  /// The result will be one of the m_soundedDistancesUnits values.
  uint32_t RoundByPresetSoundedDistancesUnits(uint32_t turnNotificationUnits) const;

  inline measurement_utils::Units GetLengthUnits() const { return m_lengthUnits; }
  inline void SetLengthUnits(measurement_utils::Units units) { m_lengthUnits = units; }
  double ConvertMetersPerSecondToUnitsPerSecond(double speedInMetersPerSecond) const;
  double ConvertUnitsToMeters(double distanceInUnits) const;
  double ConvertMetersToUnits(double distanceInMeters) const;
  void ForTestingSetNotificationTimeSecond(uint32_t time) { m_timeSeconds = time; }
};

/// \brief The Notification struct contains all the information about the next sound
/// notification to pronounce.
struct Notification
{
  /// m_distanceUnits is a distance to the turn in m_lengthUnits (meters or feet).
  /// If m_distanceUnits == 0 then the information about distance to the turn shall
  /// not be pronounced.
  uint32_t m_distanceUnits;
  uint8_t m_exitNum;
  /// if m_useThenInsteadOfDistance == true the m_distanceUnits is ignored.
  /// The word "Then" shall be pronounced intead of the distance.
  bool m_useThenInsteadOfDistance;
  CarDirection m_turnDir = CarDirection::None;
  PedestrianDirection m_turnDirPedestrian = PedestrianDirection::None;
  measurement_utils::Units m_lengthUnits;
  RouteSegment::RoadNameInfo m_nextStreetInfo;

  Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance, CarDirection turnDir,
               measurement_utils::Units lengthUnits, RouteSegment::RoadNameInfo const & nextStreetInfo)
    : m_distanceUnits(distanceUnits)
    , m_exitNum(exitNum)
    , m_useThenInsteadOfDistance(useThenInsteadOfDistance)
    , m_turnDir(turnDir)
    , m_lengthUnits(lengthUnits)
    , m_nextStreetInfo(nextStreetInfo)
  {}

  Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance, CarDirection turnDir,
               measurement_utils::Units lengthUnits)
    : m_distanceUnits(distanceUnits)
    , m_exitNum(exitNum)
    , m_useThenInsteadOfDistance(useThenInsteadOfDistance)
    , m_turnDir(turnDir)
    , m_lengthUnits(lengthUnits)
  {}

  bool operator==(Notification const & rhv) const
  {
    return m_distanceUnits == rhv.m_distanceUnits && m_exitNum == rhv.m_exitNum &&
           m_useThenInsteadOfDistance == rhv.m_useThenInsteadOfDistance && m_turnDir == rhv.m_turnDir &&
           m_turnDirPedestrian == rhv.m_turnDirPedestrian && m_lengthUnits == rhv.m_lengthUnits &&
           m_nextStreetInfo == rhv.m_nextStreetInfo;
  }

  bool IsPedestrianNotification() const { return m_turnDirPedestrian != PedestrianDirection::None; }
};

std::string DebugPrint(Notification const & turnGeom);

using PairDist = std::pair<uint32_t, char const *>;
using VecPairDist = std::vector<PairDist>;

/// @return a reference to a vector of pairs of a distance in meters and a text id.
/// All the distances are translated in supported languages and can be pronounced.
VecPairDist const & GetAllSoundedDistMeters();
/// @return a reference to a vector of pairs of a distance in feet and a text id.
/// All the distances are translated in supported languages and can be pronounced.
VecPairDist const & GetAllSoundedDistFeet();

// @TODO(vbykoianko) Now GetSoundedDistMeters/Feet() functions returns a subset of
// the result of GetAllSoundedDistMeters/Feet() functions. So GetAllSoundedDistMeters/Feet()
// returns more distances.
// After the tuning of turn sound notification is finished to do
// * remove all unnecessary distances from GetAllSoundedDistMeters/Feet().
// * remove all unnecessary string form resources. It let us to reduce the size of apk/ipa by
// 10-20KB.
// * remove GetSoundedDistMeters/Feet() and use lambda in TurnsSound::SetLengthUnits
// to convert from vector<pair<uint32_t, char const *>> to vector<uint32_t>.

/// @return distance in meters which are used for turn sound generation.
std::vector<uint32_t> const & GetSoundedDistMeters();
/// @return distance in feet which are used for turn sound generation.
std::vector<uint32_t> const & GetSoundedDistFeet();

}  // namespace sound
}  // namespace turns
}  // namespace routing
