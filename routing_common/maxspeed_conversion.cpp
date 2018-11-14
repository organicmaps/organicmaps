#include "routing_common/maxspeed_conversion.hpp"

#include "base/assert.hpp"

#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

namespace routing
{
using namespace std;
using namespace measurement_utils;

// SpeedInUnits ------------------------------------------------------------------------------------
bool SpeedInUnits::operator==(SpeedInUnits const & rhs) const
{
  return m_speed == rhs.m_speed && m_units == rhs.m_units;
}

bool SpeedInUnits::operator<(SpeedInUnits const & rhs) const
{
  return ToSpeedKmPH(m_speed, m_units) < ToSpeedKmPH(rhs.m_speed, rhs.m_units);
}

bool SpeedInUnits::IsNumeric() const
{
  return routing::IsNumeric(m_speed);
}

// Maxspeed ----------------------------------------------------------------------------------------
Maxspeed::Maxspeed(Units units, uint16_t forward, uint16_t backward)
  : m_units(units), m_forward(forward), m_backward(backward)
{
}

bool Maxspeed::operator==(Maxspeed const & rhs) const
{
  return m_units == rhs.m_units && m_forward == rhs.m_forward && m_backward == rhs.m_backward;
}

uint16_t Maxspeed::GetSpeedInUnits(bool forward) const
{
  return (forward || !IsBidirectional()) ? m_forward : m_backward;
}

uint16_t Maxspeed::GetSpeedKmPH(bool forward) const
{
  uint16_t constexpr kNoneSpeedLimitKmPH = 1000;
  uint16_t constexpr kWalkSpeedLimitKmPH = 6;

  auto speedInUnits = GetSpeedInUnits(forward);
  if (speedInUnits == kInvalidSpeed)
    return kInvalidSpeed; // That means IsValid() returns false.

  if (IsNumeric(speedInUnits))
    return ToSpeedKmPH(speedInUnits, m_units);

  // A feature is marked as a feature without any speed limits. (maxspeed=="none").
  if (kNoneMaxSpeed)
    return kNoneSpeedLimitKmPH;

  // If a feature is marked with the maxspeed=="walk" tag (speed == kWalkMaxSpeed) a driver
  // should drive with a speed of a walking person.
  if (kWalkMaxSpeed)
    return kWalkSpeedLimitKmPH;

  CHECK(false, ("Method IsNumeric() returns something wrong."));
}

// FeatureMaxspeed ---------------------------------------------------------------------------------
FeatureMaxspeed::FeatureMaxspeed(uint32_t fid, measurement_utils::Units units, uint16_t forward,
                                 uint16_t backward /* = kInvalidSpeed */) noexcept
  : m_featureId(fid), m_maxspeed(units, forward, backward)
{
}

bool FeatureMaxspeed::operator==(FeatureMaxspeed const & rhs) const
{
  return m_featureId == rhs.m_featureId && m_maxspeed == rhs.m_maxspeed;
}

SpeedInUnits FeatureMaxspeed::GetForwardSpeedInUnits() const
{
  return SpeedInUnits(GetMaxspeed().GetForward(), GetMaxspeed().GetUnits());
}

SpeedInUnits FeatureMaxspeed::GetBackwardSpeedInUnits() const
{
  return SpeedInUnits(GetMaxspeed().GetBackward(), GetMaxspeed().GetUnits());
}

// MaxspeedConverter -------------------------------------------------------------------------------
MaxspeedConverter::MaxspeedConverter()
{
  vector<tuple<SpeedMacro, uint16_t, Units>> const table = {
      // Special values.
      {SpeedMacro::Undefined, kInvalidSpeed /* speed */, Units::Metric},
      {SpeedMacro::None, kNoneMaxSpeed /* speed */, Units::Metric},
      {SpeedMacro::Walk, kWalkMaxSpeed /* speed */, Units::Metric},

      // Km per hour.
      {SpeedMacro::Speed1KmPH, 1 /* speed */, Units::Metric},
      {SpeedMacro::Speed2KmPH, 2 /* speed */, Units::Metric},
      {SpeedMacro::Speed3KmPH, 3 /* speed */, Units::Metric},
      {SpeedMacro::Speed4KmPH, 4 /* speed */, Units::Metric},
      {SpeedMacro::Speed5KmPH, 5 /* speed */, Units::Metric},
      {SpeedMacro::Speed6KmPH, 6 /* speed */, Units::Metric},
      {SpeedMacro::Speed7KmPH, 7 /* speed */, Units::Metric},
      {SpeedMacro::Speed8KmPH, 8 /* speed */, Units::Metric},
      {SpeedMacro::Speed9KmPH, 9 /* speed */, Units::Metric},
      {SpeedMacro::Speed10KmPH, 10 /* speed */, Units::Metric},
      {SpeedMacro::Speed11KmPH, 11 /* speed */, Units::Metric},
      {SpeedMacro::Speed12KmPH, 12 /* speed */, Units::Metric},
      {SpeedMacro::Speed13KmPH, 13 /* speed */, Units::Metric},
      {SpeedMacro::Speed14KmPH, 14 /* speed */, Units::Metric},
      {SpeedMacro::Speed15KmPH, 15 /* speed */, Units::Metric},
      {SpeedMacro::Speed16KmPH, 16 /* speed */, Units::Metric},
      {SpeedMacro::Speed18KmPH, 18 /* speed */, Units::Metric},
      {SpeedMacro::Speed20KmPH, 20 /* speed */, Units::Metric},
      {SpeedMacro::Speed22KmPH, 22 /* speed */, Units::Metric},
      {SpeedMacro::Speed25KmPH, 25 /* speed */, Units::Metric},
      {SpeedMacro::Speed24KmPH, 24 /* speed */, Units::Metric},
      {SpeedMacro::Speed28KmPH, 28 /* speed */, Units::Metric},
      {SpeedMacro::Speed30KmPH, 30 /* speed */, Units::Metric},
      {SpeedMacro::Speed32KmPH, 32 /* speed */, Units::Metric},
      {SpeedMacro::Speed35KmPH, 35 /* speed */, Units::Metric},
      {SpeedMacro::Speed36KmPH, 36 /* speed */, Units::Metric},
      {SpeedMacro::Speed39KmPH, 39 /* speed */, Units::Metric},
      {SpeedMacro::Speed40KmPH, 40 /* speed */, Units::Metric},
      {SpeedMacro::Speed45KmPH, 45 /* speed */, Units::Metric},
      {SpeedMacro::Speed50KmPH, 50 /* speed */, Units::Metric},
      {SpeedMacro::Speed55KmPH, 55 /* speed */, Units::Metric},
      {SpeedMacro::Speed56KmPH, 56 /* speed */, Units::Metric},
      {SpeedMacro::Speed60KmPH, 60 /* speed */, Units::Metric},
      {SpeedMacro::Speed64KmPH, 64 /* speed */, Units::Metric},
      {SpeedMacro::Speed65KmPH, 65 /* speed */, Units::Metric},
      {SpeedMacro::Speed70KmPH, 70 /* speed */, Units::Metric},
      {SpeedMacro::Speed72KmPH, 72 /* speed */, Units::Metric},
      {SpeedMacro::Speed75KmPH, 75 /* speed */, Units::Metric},
      {SpeedMacro::Speed80KmPH, 80 /* speed */, Units::Metric},
      {SpeedMacro::Speed82KmPH, 82 /* speed */, Units::Metric},
      {SpeedMacro::Speed85KmPH, 85 /* speed */, Units::Metric},
      {SpeedMacro::Speed89KmPH, 89 /* speed */, Units::Metric},
      {SpeedMacro::Speed90KmPH, 90 /* speed */, Units::Metric},
      {SpeedMacro::Speed93KmPH, 93 /* speed */, Units::Metric},
      {SpeedMacro::Speed95KmPH, 95 /* speed */, Units::Metric},
      {SpeedMacro::Speed96KmPH, 96 /* speed */, Units::Metric},
      {SpeedMacro::Speed100KmPH, 100 /* speed */, Units::Metric},
      {SpeedMacro::Speed104KmPH, 104 /* speed */, Units::Metric},
      {SpeedMacro::Speed105KmPH, 105 /* speed */, Units::Metric},
      {SpeedMacro::Speed106KmPH, 106 /* speed */, Units::Metric},
      {SpeedMacro::Speed110KmPH, 110 /* speed */, Units::Metric},
      {SpeedMacro::Speed112KmPH, 112 /* speed */, Units::Metric},
      {SpeedMacro::Speed115KmPH, 115 /* speed */, Units::Metric},
      {SpeedMacro::Speed120KmPH, 120 /* speed */, Units::Metric},
      {SpeedMacro::Speed125KmPH, 125 /* speed */, Units::Metric},
      {SpeedMacro::Speed127KmPH, 127 /* speed */, Units::Metric},
      {SpeedMacro::Speed130KmPH, 130 /* speed */, Units::Metric},
      {SpeedMacro::Speed135KmPH, 135 /* speed */, Units::Metric},
      {SpeedMacro::Speed140KmPH, 140 /* speed */, Units::Metric},
      {SpeedMacro::Speed141KmPH, 141 /* speed */, Units::Metric},
      {SpeedMacro::Speed145KmPH, 145 /* speed */, Units::Metric},
      {SpeedMacro::Speed150KmPH, 150 /* speed */, Units::Metric},
      {SpeedMacro::Speed155KmPH, 155 /* speed */, Units::Metric},
      {SpeedMacro::Speed160KmPH, 160 /* speed */, Units::Metric},
      {SpeedMacro::Speed165KmPH, 165 /* speed */, Units::Metric},
      {SpeedMacro::Speed170KmPH, 170 /* speed */, Units::Metric},
      {SpeedMacro::Speed177KmPH, 177 /* speed */, Units::Metric},
      {SpeedMacro::Speed180KmPH, 180 /* speed */, Units::Metric},
      {SpeedMacro::Speed185KmPH, 185 /* speed */, Units::Metric},
      {SpeedMacro::Speed190KmPH, 190 /* speed */, Units::Metric},
      {SpeedMacro::Speed193KmPH, 193 /* speed */, Units::Metric},
      {SpeedMacro::Speed195KmPH, 195 /* speed */, Units::Metric},
      {SpeedMacro::Speed200KmPH, 200 /* speed */, Units::Metric},
      {SpeedMacro::Speed201KmPH, 201 /* speed */, Units::Metric},
      {SpeedMacro::Speed210KmPH, 210 /* speed */, Units::Metric},
      {SpeedMacro::Speed217KmPH, 217 /* speed */, Units::Metric},
      {SpeedMacro::Speed220KmPH, 220 /* speed */, Units::Metric},
      {SpeedMacro::Speed230KmPH, 230 /* speed */, Units::Metric},
      {SpeedMacro::Speed240KmPH, 240 /* speed */, Units::Metric},
      {SpeedMacro::Speed250KmPH, 250 /* speed */, Units::Metric},
      {SpeedMacro::Speed260KmPH, 260 /* speed */, Units::Metric},
      {SpeedMacro::Speed270KmPH, 270 /* speed */, Units::Metric},
      {SpeedMacro::Speed275KmPH, 275 /* speed */, Units::Metric},
      {SpeedMacro::Speed280KmPH, 280 /* speed */, Units::Metric},
      {SpeedMacro::Speed285KmPH, 285 /* speed */, Units::Metric},
      {SpeedMacro::Speed300KmPH, 300 /* speed */, Units::Metric},
      {SpeedMacro::Speed305KmPH, 305 /* speed */, Units::Metric},
      {SpeedMacro::Speed310KmPH, 310 /* speed */, Units::Metric},
      {SpeedMacro::Speed320KmPH, 320 /* speed */, Units::Metric},
      {SpeedMacro::Speed350KmPH, 350 /* speed */, Units::Metric},
      {SpeedMacro::Speed380KmPH, 380 /* speed */, Units::Metric},

      // Miles per hour.
      {SpeedMacro::Speed3MPH, 3 /* speed */, Units::Imperial},
      {SpeedMacro::Speed4MPH, 4 /* speed */, Units::Imperial},
      {SpeedMacro::Speed5MPH, 5 /* speed */, Units::Imperial},
      {SpeedMacro::Speed6MPH, 6 /* speed */, Units::Imperial},
      {SpeedMacro::Speed7MPH, 7 /* speed */, Units::Imperial},
      {SpeedMacro::Speed8MPH, 8 /* speed */, Units::Imperial},
      {SpeedMacro::Speed9MPH, 9 /* speed */, Units::Imperial},
      {SpeedMacro::Speed10MPH, 10 /* speed */, Units::Imperial},
      {SpeedMacro::Speed12MPH, 12 /* speed */, Units::Imperial},
      {SpeedMacro::Speed13MPH, 13 /* speed */, Units::Imperial},
      {SpeedMacro::Speed14MPH, 14 /* speed */, Units::Imperial},
      {SpeedMacro::Speed15MPH, 15 /* speed */, Units::Imperial},
      {SpeedMacro::Speed17MPH, 17 /* speed */, Units::Imperial},
      {SpeedMacro::Speed18MPH, 18 /* speed */, Units::Imperial},
      {SpeedMacro::Speed19MPH, 19 /* speed */, Units::Imperial},
      {SpeedMacro::Speed20MPH, 20 /* speed */, Units::Imperial},
      {SpeedMacro::Speed24MPH, 24 /* speed */, Units::Imperial},
      {SpeedMacro::Speed25MPH, 25 /* speed */, Units::Imperial},
      {SpeedMacro::Speed30MPH, 30 /* speed */, Units::Imperial},
      {SpeedMacro::Speed35MPH, 35 /* speed */, Units::Imperial},
      {SpeedMacro::Speed40MPH, 40 /* speed */, Units::Imperial},
      {SpeedMacro::Speed45MPH, 45 /* speed */, Units::Imperial},
      {SpeedMacro::Speed50MPH, 50 /* speed */, Units::Imperial},
      {SpeedMacro::Speed55MPH, 55 /* speed */, Units::Imperial},
      {SpeedMacro::Speed59MPH, 59 /* speed */, Units::Imperial},
      {SpeedMacro::Speed60MPH, 60 /* speed */, Units::Imperial},
      {SpeedMacro::Speed65MPH, 65 /* speed */, Units::Imperial},
      {SpeedMacro::Speed70MPH, 70 /* speed */, Units::Imperial},
      {SpeedMacro::Speed75MPH, 75 /* speed */, Units::Imperial},
      {SpeedMacro::Speed79MPH, 79 /* speed */, Units::Imperial},
      {SpeedMacro::Speed80MPH, 80 /* speed */, Units::Imperial},
      {SpeedMacro::Speed85MPH, 85 /* speed */, Units::Imperial},
      {SpeedMacro::Speed90MPH, 90 /* speed */, Units::Imperial},
      {SpeedMacro::Speed95MPH, 95 /* speed */, Units::Imperial},
      {SpeedMacro::Speed100MPH, 100 /* speed */, Units::Imperial},
      {SpeedMacro::Speed105MPH, 105 /* speed */, Units::Imperial},
      {SpeedMacro::Speed110MPH, 110 /* speed */, Units::Imperial},
      {SpeedMacro::Speed115MPH, 115 /* speed */, Units::Imperial},
      {SpeedMacro::Speed120MPH, 120 /* speed */, Units::Imperial},
      {SpeedMacro::Speed125MPH, 125 /* speed */, Units::Imperial},
  };

  for (auto const & e : table)
    m_macroToSpeed[static_cast<uint8_t>(get<0>(e))] = SpeedInUnits(get<1>(e), get<2>(e));

  CHECK_EQUAL(static_cast<uint8_t>(SpeedMacro::Undefined), 0, ());
  m_speedToMacro.insert(make_pair(SpeedInUnits(kInvalidSpeed, Units::Metric), SpeedMacro::Undefined));
  for (size_t i = 1; i < numeric_limits<uint8_t>::max(); ++i)
  {
    auto const & speed = m_macroToSpeed[i];
    if (!speed.IsValid())
      continue;

    m_speedToMacro.insert(make_pair(speed, static_cast<SpeedMacro>(i)));
  }
}

SpeedInUnits MaxspeedConverter::MacroToSpeed(SpeedMacro macro) const
{
  return m_macroToSpeed[static_cast<uint8_t>(macro)];
}

SpeedMacro MaxspeedConverter::SpeedToMacro(SpeedInUnits const & speed) const
{
  auto const it = m_speedToMacro.find(speed);
  if (it == m_speedToMacro.cend())
  {
    // Note. On the feature emitting step we emit a maxspeed value with maxspeed:forward and
    // maxspeed:backward only if forward and backward have the same units or one or two of them
    // have a special value ("none", "walk"). So if forward maxspeed is in imperial units
    // and backward maxspeed has a special value (like "none"), we may get a line is csv
    // like this "100,Imperial,100,65534". Conditions below is written to process such edge cases
    // correctly.
    if (speed == SpeedInUnits(kNoneMaxSpeed, Units::Imperial))
      return SpeedMacro::None;
    if (speed == SpeedInUnits(kWalkMaxSpeed, Units::Imperial))
      return SpeedMacro::Walk;

    return SpeedMacro::Undefined;
  }

  return it->second;
}

bool MaxspeedConverter::IsValidMacro(uint8_t macro) const
{
  CHECK_LESS(macro, numeric_limits<uint8_t>::max(), ());
  return m_macroToSpeed[macro].IsValid();
}

// static
MaxspeedConverter const & MaxspeedConverter::Instance()
{
  static const MaxspeedConverter inst;
  return inst;
}

MaxspeedConverter const & GetMaxspeedConverter()
{
  return MaxspeedConverter::Instance();
}

bool HaveSameUnits(SpeedInUnits const & lhs, SpeedInUnits const & rhs)
{
  return lhs.GetUnits() == rhs.GetUnits() || !lhs.IsNumeric() || !rhs.IsNumeric();
}

bool IsFeatureIdLess(FeatureMaxspeed const & lhs, FeatureMaxspeed const & rhs)
{
  return lhs.IsFeatureIdLess(rhs);
}

bool IsNumeric(uint16_t speed)
{
  return speed != kNoneMaxSpeed && speed != kWalkMaxSpeed && speed != kInvalidSpeed;
}

string DebugPrint(Maxspeed maxspeed)
{
  ostringstream oss;
  oss << "Maxspeed [ m_units:" << DebugPrint(maxspeed.GetUnits())
      << " m_forward:" << maxspeed.GetForward()
      << " m_backward:" << maxspeed.GetBackward() << " ]";
  return oss.str();
}

string DebugPrint(SpeedMacro maxspeed)
{
  ostringstream oss;
  oss << "SpeedMacro:" << static_cast<int>(maxspeed) << " Decoded:"
      << DebugPrint(GetMaxspeedConverter().MacroToSpeed(maxspeed));
  return oss.str();
}

string DebugPrint(SpeedInUnits const & speed)
{
  ostringstream oss;
  oss << "SpeedInUnits [ m_speed == " << speed.GetSpeed()
      << ", m_units:" << DebugPrint(speed.GetUnits()) << " ]";
  return oss.str();
}

string DebugPrint(FeatureMaxspeed const & featureMaxspeed)
{
  ostringstream oss;
  oss << "FeatureMaxspeed [ m_featureId:" << featureMaxspeed.GetFeatureId()
      << " m_maxspeed:" << DebugPrint(featureMaxspeed.GetMaxspeed()) << " ]";
  return oss.str();
}
}  // namespace routing
