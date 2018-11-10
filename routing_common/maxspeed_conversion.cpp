#include "routing_common/maxspeed_conversion.hpp"

#include <sstream>
#include <utility>

namespace routing
{
using namespace measurement_utils;

// SpeedInUnits ------------------------------------------------------------------------------------
bool SpeedInUnits::operator==(SpeedInUnits const & rhs) const
{
  return m_speed == rhs.m_speed && m_units == rhs.m_units;
}

bool SpeedInUnits::operator<(SpeedInUnits const & rhs) const
{
  return (m_units == Units::Metric ? m_speed : MphToKmph(m_speed)) <
         (rhs.m_units == Units::Metric ? rhs.m_speed : MphToKmph(rhs.m_speed));
}

bool SpeedInUnits::IsNumeric() const
{
  return m_speed != kNoneMaxSpeed && m_speed != kWalkMaxSpeed && m_speed != kInvalidSpeed;
}

// FeatureMaxspeed ---------------------------------------------------------------------------------
FeatureMaxspeed::FeatureMaxspeed(uint32_t fid, measurement_utils::Units units, uint16_t forward,
                                 uint16_t backward /* = kInvalidSpeed */) noexcept
  : m_featureId(fid), m_maxspeed({units, forward, backward})
{
}

bool FeatureMaxspeed::operator==(FeatureMaxspeed const & rhs) const
{
  return m_featureId == rhs.m_featureId && m_maxspeed == rhs.m_maxspeed;
}

SpeedInUnits FeatureMaxspeed::GetForwardSpeedInUnits() const
{
  return SpeedInUnits(GetMaxspeed().m_forward, GetMaxspeed().m_units);
}

SpeedInUnits FeatureMaxspeed::GetBackwardSpeedInUnits() const
{
  return SpeedInUnits(GetMaxspeed().m_backward, GetMaxspeed().m_units);
}

// MaxspeedConverter -------------------------------------------------------------------------------
MaxspeedConverter::MaxspeedConverter()
{
  // Special values.
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Undefined)] = SpeedInUnits(kInvalidSpeed, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::None)] = SpeedInUnits(kNoneMaxSpeed, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Walk)] = SpeedInUnits(kWalkMaxSpeed, Units::Metric);

  // Km per hour.
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed1kph)] = SpeedInUnits(1, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed2kph)] = SpeedInUnits(2, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed3kph)] = SpeedInUnits(3, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed4kph)] = SpeedInUnits(4, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed5kph)] = SpeedInUnits(5, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed6kph)] = SpeedInUnits(6, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed7kph)] = SpeedInUnits(7, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed8kph)] = SpeedInUnits(8, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed9kph)] = SpeedInUnits(9, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed10kph)] = SpeedInUnits(10, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed11kph)] = SpeedInUnits(11, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed12kph)] = SpeedInUnits(12, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed13kph)] = SpeedInUnits(13, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed14kph)] = SpeedInUnits(14, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed15kph)] = SpeedInUnits(15, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed16kph)] = SpeedInUnits(16, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed18kph)] = SpeedInUnits(18, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed20kph)] = SpeedInUnits(20, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed22kph)] = SpeedInUnits(22, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed25kph)] = SpeedInUnits(25, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed24kph)] = SpeedInUnits(24, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed28kph)] = SpeedInUnits(28, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed30kph)] = SpeedInUnits(30, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed32kph)] = SpeedInUnits(32, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed35kph)] = SpeedInUnits(35, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed36kph)] = SpeedInUnits(36, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed39kph)] = SpeedInUnits(39, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed40kph)] = SpeedInUnits(40, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed45kph)] = SpeedInUnits(45, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed50kph)] = SpeedInUnits(50, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed55kph)] = SpeedInUnits(55, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed56kph)] = SpeedInUnits(56, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed60kph)] = SpeedInUnits(60, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed64kph)] = SpeedInUnits(64, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed65kph)] = SpeedInUnits(65, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed70kph)] = SpeedInUnits(70, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed72kph)] = SpeedInUnits(72, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed75kph)] = SpeedInUnits(75, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed80kph)] = SpeedInUnits(80, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed82kph)] = SpeedInUnits(82, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed85kph)] = SpeedInUnits(85, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed89kph)] = SpeedInUnits(89, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed90kph)] = SpeedInUnits(90, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed93kph)] = SpeedInUnits(93, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed95kph)] = SpeedInUnits(95, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed96kph)] = SpeedInUnits(96, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed100kph)] = SpeedInUnits(100, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed104kph)] = SpeedInUnits(104, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed105kph)] = SpeedInUnits(105, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed106kph)] = SpeedInUnits(106, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed110kph)] = SpeedInUnits(110, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed112kph)] = SpeedInUnits(112, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed115kph)] = SpeedInUnits(115, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed120kph)] = SpeedInUnits(120, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed125kph)] = SpeedInUnits(125, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed127kph)] = SpeedInUnits(127, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed130kph)] = SpeedInUnits(130, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed135kph)] = SpeedInUnits(135, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed140kph)] = SpeedInUnits(140, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed141kph)] = SpeedInUnits(141, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed145kph)] = SpeedInUnits(145, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed150kph)] = SpeedInUnits(150, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed155kph)] = SpeedInUnits(155, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed160kph)] = SpeedInUnits(160, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed165kph)] = SpeedInUnits(165, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed170kph)] = SpeedInUnits(170, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed177kph)] = SpeedInUnits(177, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed180kph)] = SpeedInUnits(180, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed185kph)] = SpeedInUnits(185, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed190kph)] = SpeedInUnits(190, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed193kph)] = SpeedInUnits(193, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed195kph)] = SpeedInUnits(195, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed200kph)] = SpeedInUnits(200, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed201kph)] = SpeedInUnits(201, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed210kph)] = SpeedInUnits(210, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed217kph)] = SpeedInUnits(217, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed220kph)] = SpeedInUnits(220, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed230kph)] = SpeedInUnits(230, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed240kph)] = SpeedInUnits(240, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed250kph)] = SpeedInUnits(250, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed260kph)] = SpeedInUnits(260, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed270kph)] = SpeedInUnits(270, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed275kph)] = SpeedInUnits(275, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed280kph)] = SpeedInUnits(280, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed285kph)] = SpeedInUnits(285, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed300kph)] = SpeedInUnits(300, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed305kph)] = SpeedInUnits(305, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed310kph)] = SpeedInUnits(310, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed320kph)] = SpeedInUnits(320, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed350kph)] = SpeedInUnits(350, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed380kph)] = SpeedInUnits(380, Units::Metric);

  // Mile per hours.
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed3mph)] = SpeedInUnits(3, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed4mph)] = SpeedInUnits(4, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed5mph)] = SpeedInUnits(5, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed6mph)] = SpeedInUnits(6, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed7mph)] = SpeedInUnits(7, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed8mph)] = SpeedInUnits(8, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed9mph)] = SpeedInUnits(9, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed10mph)] = SpeedInUnits(10, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed12mph)] = SpeedInUnits(12, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed13mph)] = SpeedInUnits(13, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed14mph)] = SpeedInUnits(14, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed15mph)] = SpeedInUnits(15, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed17mph)] = SpeedInUnits(17, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed18mph)] = SpeedInUnits(18, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed19mph)] = SpeedInUnits(19, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed20mph)] = SpeedInUnits(20, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed24mph)] = SpeedInUnits(24, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed25mph)] = SpeedInUnits(25, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed30mph)] = SpeedInUnits(30, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed35mph)] = SpeedInUnits(35, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed40mph)] = SpeedInUnits(40, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed45mph)] = SpeedInUnits(45, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed50mph)] = SpeedInUnits(50, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed55mph)] = SpeedInUnits(55, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed59mph)] = SpeedInUnits(59, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed60mph)] = SpeedInUnits(60, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed65mph)] = SpeedInUnits(65, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed70mph)] = SpeedInUnits(70, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed75mph)] = SpeedInUnits(75, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed79mph)] = SpeedInUnits(79, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed80mph)] = SpeedInUnits(80, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed85mph)] = SpeedInUnits(85, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed90mph)] = SpeedInUnits(90, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed95mph)] = SpeedInUnits(95, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed100mph)] = SpeedInUnits(100, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed105mph)] = SpeedInUnits(105, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed110mph)] = SpeedInUnits(110, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed115mph)] = SpeedInUnits(115, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed120mph)] = SpeedInUnits(120, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(SpeedMacro::Speed125mph)] = SpeedInUnits(125, Units::Imperial);

  m_speedToMacro.insert(std::make_pair(SpeedInUnits(kInvalidSpeed, Units::Metric), SpeedMacro::Undefined));
  for (size_t i = 1; i < std::numeric_limits<uint8_t>::max(); ++i)
  {
    auto const & speed = m_macroToSpeed[i];
    if (!speed.IsValid())
      continue;

    m_speedToMacro.insert(std::make_pair(speed, static_cast<SpeedMacro>(i)));
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
  return m_macroToSpeed[macro].IsValid();
}

MaxspeedConverter const & GetMaxspeedConverter()
{
  static const MaxspeedConverter inst;
  return inst;
}

bool HaveSameUnits(SpeedInUnits const & lhs, SpeedInUnits const & rhs)
{
  return lhs.m_units == rhs.m_units || !lhs.IsNumeric() || !rhs.IsNumeric();
}

std::string DebugPrint(Maxspeed maxspeed)
{
  std::ostringstream oss;
  oss << "Maxspeed [ m_units:" << DebugPrint(maxspeed.m_units)
      << " m_forward:" << maxspeed.m_forward
      << " m_backward:" << maxspeed.m_backward << " ]";
  return oss.str();
}

std::string DebugPrint(SpeedMacro maxspeed)
{
  std::ostringstream oss;
  oss << "SpeedMacro:" << static_cast<int>(maxspeed) << " Decoded:"
      << DebugPrint(GetMaxspeedConverter().MacroToSpeed(maxspeed));
  return oss.str();
}

std::string DebugPrint(SpeedInUnits const & speed)
{
  std::ostringstream oss;
  oss << "SpeedInUnits [ m_speed == " << speed.m_speed
      << ", m_units:" << DebugPrint(speed.m_units) << " ]";
  return oss.str();
}

std::string DebugPrint(FeatureMaxspeed const & featureMaxspeed)
{
  std::ostringstream oss;
  oss << "FeatureMaxspeed [ m_featureId:" << featureMaxspeed.GetFeatureId()
      << " m_maxspeed:" << DebugPrint(featureMaxspeed.GetMaxspeed()) << " ]";
  return oss.str();
}
}  // namespace routing
