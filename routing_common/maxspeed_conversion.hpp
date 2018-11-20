#pragma once

#include "platform/measurement_utils.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <string>

namespace routing
{
/// \brief This enum class contains the most popular maxspeed values according to
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
  Speed1KmPH = 10,
  Speed2KmPH,
  Speed3KmPH,
  Speed4KmPH,
  Speed5KmPH,
  Speed6KmPH,
  Speed7KmPH,
  Speed8KmPH,
  Speed9KmPH,
  Speed10KmPH,
  Speed11KmPH,
  Speed12KmPH,
  Speed13KmPH,
  Speed14KmPH,
  Speed15KmPH,
  Speed16KmPH,
  Speed18KmPH,
  Speed20KmPH,
  Speed22KmPH,
  Speed25KmPH,
  Speed24KmPH,
  Speed28KmPH,
  Speed30KmPH,
  Speed32KmPH,
  Speed35KmPH,
  Speed36KmPH,
  Speed39KmPH,
  Speed40KmPH,
  Speed45KmPH,
  Speed50KmPH,
  Speed55KmPH,
  Speed56KmPH,
  Speed60KmPH,
  Speed64KmPH,
  Speed65KmPH,
  Speed70KmPH,
  Speed72KmPH,
  Speed75KmPH,
  Speed80KmPH,
  Speed82KmPH,
  Speed85KmPH,
  Speed89KmPH,
  Speed90KmPH,
  Speed93KmPH,
  Speed95KmPH,
  Speed96KmPH,
  Speed100KmPH,
  Speed104KmPH,
  Speed105KmPH,
  Speed106KmPH,
  Speed110KmPH,
  Speed112KmPH,
  Speed115KmPH,
  Speed120KmPH,
  Speed125KmPH,
  Speed127KmPH,
  Speed130KmPH,
  Speed135KmPH,
  Speed140KmPH,
  Speed141KmPH,
  Speed145KmPH,
  Speed150KmPH,
  Speed155KmPH,
  Speed160KmPH,
  Speed165KmPH,
  Speed170KmPH,
  Speed177KmPH,
  Speed180KmPH,
  Speed185KmPH,
  Speed190KmPH,
  Speed193KmPH,
  Speed195KmPH,
  Speed200KmPH,
  Speed201KmPH,
  Speed210KmPH,
  Speed217KmPH,
  Speed220KmPH,
  Speed230KmPH,
  Speed240KmPH,
  Speed250KmPH,
  Speed260KmPH,
  Speed270KmPH,
  Speed275KmPH,
  Speed280KmPH,
  Speed285KmPH,
  Speed300KmPH,
  Speed305KmPH,
  Speed310KmPH,
  Speed320KmPH,
  Speed350KmPH,
  Speed380KmPH,

  // Miles per hour.
  Speed3MPH = 110,
  Speed4MPH,
  Speed5MPH,
  Speed6MPH,
  Speed7MPH,
  Speed8MPH,
  Speed9MPH,
  Speed10MPH,
  Speed12MPH,
  Speed13MPH,
  Speed14MPH,
  Speed15MPH,
  Speed17MPH,
  Speed18MPH,
  Speed19MPH,
  Speed20MPH,
  Speed24MPH,
  Speed25MPH,
  Speed30MPH,
  Speed35MPH,
  Speed40MPH,
  Speed45MPH,
  Speed50MPH,
  Speed55MPH,
  Speed59MPH,
  Speed60MPH,
  Speed65MPH,
  Speed70MPH,
  Speed75MPH,
  Speed79MPH,
  Speed80MPH,
  Speed85MPH,
  Speed90MPH,
  Speed95MPH,
  Speed100MPH,
  Speed105MPH,
  Speed110MPH,
  Speed115MPH,
  Speed120MPH,
  Speed125MPH,
};

uint16_t constexpr kInvalidSpeed = std::numeric_limits<uint16_t>::max();
uint16_t constexpr kNoneMaxSpeed = std::numeric_limits<uint16_t>::max() - 1;
uint16_t constexpr kWalkMaxSpeed = std::numeric_limits<uint16_t>::max() - 2;

class SpeedInUnits
{
public:
  SpeedInUnits() = default;
  SpeedInUnits(uint16_t speed, measurement_utils::Units units) noexcept : m_speed(speed), m_units(units) {}

  void SetSpeed(uint16_t speed) { m_speed = speed; }
  void SetUnits(measurement_utils::Units units) { m_units = units; }

  uint16_t GetSpeed() const { return m_speed; }
  measurement_utils::Units GetUnits() const { return m_units; }

  bool operator==(SpeedInUnits const & rhs) const;
  bool operator<(SpeedInUnits const & rhs) const;

  bool IsNumeric() const;
  bool IsValid() const { return m_speed != kInvalidSpeed; }

private:
  // Speed in km per hour or mile per hour depends on m_units value.
  uint16_t m_speed = kInvalidSpeed;
  // |m_units| is undefined in case of SpeedMacro::None and SpeedMacro::Walk.
  measurement_utils::Units m_units = measurement_utils::Units::Metric;
};

class Maxspeed
{
public:
  Maxspeed() = default;
  Maxspeed(measurement_utils::Units units, uint16_t forward, uint16_t backward);

  bool operator==(Maxspeed const & rhs) const;

  void SetUnits(measurement_utils::Units units) { m_units = units; }
  void SetForward(uint16_t forward) { m_forward = forward; }
  void SetBackward(uint16_t backward) { m_backward = backward; }

  measurement_utils::Units GetUnits() const { return m_units; }
  uint16_t GetForward() const { return m_forward; }
  uint16_t GetBackward() const { return m_backward; }

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

private:
  measurement_utils::Units m_units = measurement_utils::Units::Metric;
  // Speed in km per hour or mile per hour depends on |m_units|.
  uint16_t m_forward = kInvalidSpeed;
  // Speed in km per hour or mile per hour depends on |m_units|. If |m_backward| == kInvalidSpeed
  // |m_forward| speed should be used for the both directions.
  uint16_t m_backward = kInvalidSpeed;
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

  bool operator==(FeatureMaxspeed const & rhs) const;
  bool IsFeatureIdLess(FeatureMaxspeed const & rhs) const { return m_featureId < rhs.m_featureId; }

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
public:
  SpeedInUnits MacroToSpeed(SpeedMacro macro) const;
  SpeedMacro SpeedToMacro(SpeedInUnits const & speed) const;

  /// \returns true if |macro| can be cast to a valid value of SpeedMacro emum class.
  /// \note SpeedMacro::Undefined value and all values from 1 to 256 which are not present
  /// in SpeedMacro enum class are considered as an invalid.
  bool IsValidMacro(uint8_t macro) const;

  static MaxspeedConverter const & Instance();

private:
  MaxspeedConverter();

  std::array<SpeedInUnits, std::numeric_limits<uint8_t>::max()> m_macroToSpeed;
  std::map<SpeedInUnits, SpeedMacro> m_speedToMacro;
};

MaxspeedConverter const & GetMaxspeedConverter();
bool HaveSameUnits(SpeedInUnits const & lhs, SpeedInUnits const & rhs);
bool IsFeatureIdLess(FeatureMaxspeed const & lhs, FeatureMaxspeed const & rhs);

/// \returns false if |speed| is equal to |kInvalidSpeed|, |kNoneMaxSpeed| or
/// |kWalkMaxSpeed|.
/// \param speed in km per hour or mile per hour.
bool IsNumeric(uint16_t speed);

std::string DebugPrint(Maxspeed maxspeed);
std::string DebugPrint(SpeedMacro maxspeed);
std::string DebugPrint(SpeedInUnits const & speed);
std::string DebugPrint(FeatureMaxspeed const & featureMaxspeed);
}  // namespace routing
