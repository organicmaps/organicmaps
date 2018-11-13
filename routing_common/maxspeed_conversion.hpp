#pragma once

#include "platform/measurement_utils.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <string>

namespace routing
{
/// \brief This enum class contains most popular maxspeed value according to
/// https://taginfo.openstreetmap.org/keys/maxspeed#values.
/// \note Value of this enum is saved to mwm. So they should not be changed because of backward
/// compatibility. But it's possible to add some new values to this enum.
enum class SpeedMacro : uint8_t
{
  // Special values.
  Undefined,
  None,       // No maxspeed restriction (E.g. a motorway in Germany).
  Walk,       // Driver should move as a walking person.

  // Km per hour.
  Speed1kph = 10,
  Speed2kph,
  Speed3kph,
  Speed4kph,
  Speed5kph,
  Speed6kph,
  Speed7kph,
  Speed8kph,
  Speed9kph,
  Speed10kph,
  Speed11kph,
  Speed12kph,
  Speed13kph,
  Speed14kph,
  Speed15kph,
  Speed16kph,
  Speed18kph,
  Speed20kph,
  Speed22kph,
  Speed25kph,
  Speed24kph,
  Speed28kph,
  Speed30kph,
  Speed32kph,
  Speed35kph,
  Speed36kph,
  Speed39kph,
  Speed40kph,
  Speed45kph,
  Speed50kph,
  Speed55kph,
  Speed56kph,
  Speed60kph,
  Speed64kph,
  Speed65kph,
  Speed70kph,
  Speed72kph,
  Speed75kph,
  Speed80kph,
  Speed82kph,
  Speed85kph,
  Speed89kph,
  Speed90kph,
  Speed93kph,
  Speed95kph,
  Speed96kph,
  Speed100kph,
  Speed104kph,
  Speed105kph,
  Speed106kph,
  Speed110kph,
  Speed112kph,
  Speed115kph,
  Speed120kph,
  Speed125kph,
  Speed127kph,
  Speed130kph,
  Speed135kph,
  Speed140kph,
  Speed141kph,
  Speed145kph,
  Speed150kph,
  Speed155kph,
  Speed160kph,
  Speed165kph,
  Speed170kph,
  Speed177kph,
  Speed180kph,
  Speed185kph,
  Speed190kph,
  Speed193kph,
  Speed195kph,
  Speed200kph,
  Speed201kph,
  Speed210kph,
  Speed217kph,
  Speed220kph,
  Speed230kph,
  Speed240kph,
  Speed250kph,
  Speed260kph,
  Speed270kph,
  Speed275kph,
  Speed280kph,
  Speed285kph,
  Speed300kph,
  Speed305kph,
  Speed310kph,
  Speed320kph,
  Speed350kph,
  Speed380kph,

  // Mile per hours.
  Speed3mph = 110,
  Speed4mph,
  Speed5mph,
  Speed6mph,
  Speed7mph,
  Speed8mph,
  Speed9mph,
  Speed10mph,
  Speed12mph,
  Speed13mph,
  Speed14mph,
  Speed15mph,
  Speed17mph,
  Speed18mph,
  Speed19mph,
  Speed20mph,
  Speed24mph,
  Speed25mph,
  Speed30mph,
  Speed35mph,
  Speed40mph,
  Speed45mph,
  Speed50mph,
  Speed55mph,
  Speed59mph,
  Speed60mph,
  Speed65mph,
  Speed70mph,
  Speed75mph,
  Speed79mph,
  Speed80mph,
  Speed85mph,
  Speed90mph,
  Speed95mph,
  Speed100mph,
  Speed105mph,
  Speed110mph,
  Speed115mph,
  Speed120mph,
  Speed125mph,
};

uint16_t constexpr kInvalidSpeed = std::numeric_limits<uint16_t>::max();
uint16_t constexpr kNoneMaxSpeed = std::numeric_limits<uint16_t>::max() - 1;
uint16_t constexpr kWalkMaxSpeed = std::numeric_limits<uint16_t>::max() - 2;

struct SpeedInUnits
{
  SpeedInUnits() = default;
  SpeedInUnits(uint16_t speed, measurement_utils::Units units) noexcept : m_speed(speed), m_units(units) {}

  bool operator==(SpeedInUnits const & rhs) const;
  bool operator<(SpeedInUnits const & rhs) const;

  bool IsNumeric() const;
  bool IsValid() const { return m_speed != kInvalidSpeed; }

  // Speed in km per hour or mile per hour depends on m_units value.
  uint16_t m_speed = kInvalidSpeed;
  // |m_units| is undefined in case of SpeedMacro::None and SpeedMacro::Walk.
  measurement_utils::Units m_units = measurement_utils::Units::Metric;
};

struct Maxspeed
{
  measurement_utils::Units m_units = measurement_utils::Units::Metric;
  // Speed in km per hour or mile per hour depends on |m_units|.
  uint16_t m_forward = kInvalidSpeed;
  // Speed in km per hour or mile per hour depends on |m_units|. If |m_backward| == kInvalidSpeed
  // |m_forward| speed should be used for the both directions.
  uint16_t m_backward = kInvalidSpeed;

  bool operator==(Maxspeed const & rhs) const;

  bool IsValid() const { return m_forward != kInvalidSpeed; }
  /// \returns true if Maxspeed is considered as Bidirectional(). It means different
  /// speed is set for forward and backward direction. Otherwise returns false. It means
  /// |m_forward| speed should be used for the both directions.
  bool IsBidirectional() const { return IsValid() && m_backward != kInvalidSpeed; }

  /// \brief returns speed according to |m_units|. |kInvalidSpeed|, |kNoneMaxSpeed| or
  /// |kWalkMaxSpeed| may be returned.
  uint16_t GetSpeedInUnits(bool forward) const;

  /// \brief returns speed in km per hour. If it's not valid |kInvalidSpeed| is
  /// returned. Otherwise forward or backward speed in km per hour is returned. |kNoneMaxSpeed| and
  /// |kWalkMaxSpeed| are converted to some numbers.
  uint16_t GetSpeedKmPH(bool forward) const;
};

/// \brief Feature id and corresponding maxspeed tag value. |m_forward| and |m_backward| fields
/// reflect the fact that a feature may have different maxspeed tag value for different directions.
/// If |m_backward| is invalid it means that |m_forward| tag contains maxspeed for
/// the both directions. If a feature has maxspeed forward and maxspeed backward in different units
/// it's considered as an invalid one and it's not saved into mwm.
class FeatureMaxspeed
{
public:
  FeatureMaxspeed(uint32_t fid, measurement_utils::Units units, uint16_t forward,
                  uint16_t backward = kInvalidSpeed) noexcept;

  /// \note operator==() and operator<() do not correspond to each other.
  bool operator==(FeatureMaxspeed const & rhs) const;
  bool operator<(FeatureMaxspeed const & rhs) const { return m_featureId < rhs.m_featureId; }

  bool IsValid() const { return m_maxspeed.IsValid(); }
  bool IsBidirectional() const { return m_maxspeed.IsBidirectional(); }

  uint32_t GetFeatureId() const { return m_featureId; }
  Maxspeed const & GetMaxspeed() const { return m_maxspeed; }

  SpeedInUnits GetForwardSpeedInUnits() const;
  SpeedInUnits GetBackwardSpeedInUnits() const;

private:
  uint32_t m_featureId = 0;
  Maxspeed m_maxspeed;
};

class MaxspeedConverter
{
  friend MaxspeedConverter const & GetMaxspeedConverter();
public:
  MaxspeedConverter();

  SpeedInUnits MacroToSpeed(SpeedMacro macro) const;
  SpeedMacro SpeedToMacro(SpeedInUnits const & speed) const;

  /// \returns true if |macro| can be cast to a valid value of SpeedMacro emum class.
  /// \note SpeedMacro::Undefined value and all values from 1 to 256 which are not present
  /// in SpeedMacro enum class are considered as an invalid.
  bool IsValidMacro(uint8_t macro) const;

private:
  std::array<SpeedInUnits, std::numeric_limits<uint8_t>::max()> m_macroToSpeed;
  std::map<SpeedInUnits, SpeedMacro> m_speedToMacro;
};

MaxspeedConverter const & GetMaxspeedConverter();
bool HaveSameUnits(SpeedInUnits const & lhs, SpeedInUnits const & rhs);

/// \returns false if |speed| is equal to |kInvalidSpeed|, |kNoneMaxSpeed| or
/// |kWalkMaxSpeed|.
/// \param speed in km per hour or mile per hour.
bool IsNumeric(uint16_t speed);

std::string DebugPrint(Maxspeed maxspeed);
std::string DebugPrint(SpeedMacro maxspeed);
std::string DebugPrint(SpeedInUnits const & speed);
std::string DebugPrint(FeatureMaxspeed const & featureMaxspeed);
}  // namespace routing
