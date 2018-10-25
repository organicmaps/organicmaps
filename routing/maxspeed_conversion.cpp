#include "routing/maxspeed_conversion.hpp"

#include <sstream>
#include <string>
#include <utility>

namespace routing
{
using namespace measurement_utils;

// SpeedInUnits ------------------------------------------------------------------------------------
bool SpeedInUnits::operator==(SpeedInUnits const & s) const
{
  return m_speed == s.m_speed && m_units == s.m_units;
}

bool SpeedInUnits::operator<(SpeedInUnits const & s) const
{
  if (m_speed != s.m_speed)
    return m_speed < s.m_speed;
  return m_units < s.m_units;
}

bool SpeedInUnits::IsNumeric() const
{
  return m_speed != kNoneMaxSpeed && m_speed != kWalkMaxSpeed && m_speed != kInvalidSpeed;
}

// MaxspeedConverter -------------------------------------------------------------------------------
MaxspeedConverter::MaxspeedConverter()
{
  // Special values.
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Undefined)] = SpeedInUnits(kInvalidSpeed, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::None)] = SpeedInUnits(kNoneMaxSpeed, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Walk)] = SpeedInUnits(kWalkMaxSpeed, Units::Metric);

  // Km per hour.
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed1kph)] = SpeedInUnits(1, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed2kph)] = SpeedInUnits(2, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed3kph)] = SpeedInUnits(3, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed4kph)] = SpeedInUnits(4, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed5kph)] = SpeedInUnits(5, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed6kph)] = SpeedInUnits(6, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed7kph)] = SpeedInUnits(7, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed8kph)] = SpeedInUnits(8, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed9kph)] = SpeedInUnits(9, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed10kph)] = SpeedInUnits(10, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed11kph)] = SpeedInUnits(11, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed12kph)] = SpeedInUnits(12, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed13kph)] = SpeedInUnits(13, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed14kph)] = SpeedInUnits(14, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed15kph)] = SpeedInUnits(15, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed16kph)] = SpeedInUnits(16, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed18kph)] = SpeedInUnits(18, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed20kph)] = SpeedInUnits(20, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed22kph)] = SpeedInUnits(22, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed25kph)] = SpeedInUnits(25, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed24kph)] = SpeedInUnits(24, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed28kph)] = SpeedInUnits(28, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed30kph)] = SpeedInUnits(30, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed32kph)] = SpeedInUnits(32, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed35kph)] = SpeedInUnits(35, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed36kph)] = SpeedInUnits(36, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed39kph)] = SpeedInUnits(39, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed40kph)] = SpeedInUnits(40, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed45kph)] = SpeedInUnits(45, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed50kph)] = SpeedInUnits(50, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed55kph)] = SpeedInUnits(55, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed56kph)] = SpeedInUnits(56, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed60kph)] = SpeedInUnits(60, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed64kph)] = SpeedInUnits(64, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed65kph)] = SpeedInUnits(65, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed70kph)] = SpeedInUnits(70, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed72kph)] = SpeedInUnits(72, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed75kph)] = SpeedInUnits(75, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed80kph)] = SpeedInUnits(80, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed82kph)] = SpeedInUnits(82, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed85kph)] = SpeedInUnits(85, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed89kph)] = SpeedInUnits(89, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed90kph)] = SpeedInUnits(90, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed93kph)] = SpeedInUnits(93, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed95kph)] = SpeedInUnits(95, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed96kph)] = SpeedInUnits(96, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed100kph)] = SpeedInUnits(100, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed104kph)] = SpeedInUnits(104, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed105kph)] = SpeedInUnits(105, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed106kph)] = SpeedInUnits(106, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed110kph)] = SpeedInUnits(110, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed112kph)] = SpeedInUnits(112, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed115kph)] = SpeedInUnits(115, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed120kph)] = SpeedInUnits(120, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed125kph)] = SpeedInUnits(125, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed127kph)] = SpeedInUnits(127, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed130kph)] = SpeedInUnits(130, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed135kph)] = SpeedInUnits(135, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed140kph)] = SpeedInUnits(140, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed141kph)] = SpeedInUnits(141, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed145kph)] = SpeedInUnits(145, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed150kph)] = SpeedInUnits(150, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed155kph)] = SpeedInUnits(155, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed160kph)] = SpeedInUnits(160, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed165kph)] = SpeedInUnits(165, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed170kph)] = SpeedInUnits(170, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed177kph)] = SpeedInUnits(177, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed180kph)] = SpeedInUnits(180, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed185kph)] = SpeedInUnits(185, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed190kph)] = SpeedInUnits(190, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed193kph)] = SpeedInUnits(193, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed195kph)] = SpeedInUnits(195, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed200kph)] = SpeedInUnits(200, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed201kph)] = SpeedInUnits(201, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed210kph)] = SpeedInUnits(210, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed217kph)] = SpeedInUnits(217, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed220kph)] = SpeedInUnits(220, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed230kph)] = SpeedInUnits(230, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed240kph)] = SpeedInUnits(240, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed250kph)] = SpeedInUnits(250, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed260kph)] = SpeedInUnits(260, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed270kph)] = SpeedInUnits(270, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed275kph)] = SpeedInUnits(275, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed280kph)] = SpeedInUnits(280, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed285kph)] = SpeedInUnits(285, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed300kph)] = SpeedInUnits(300, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed305kph)] = SpeedInUnits(305, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed310kph)] = SpeedInUnits(310, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed320kph)] = SpeedInUnits(320, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed350kph)] = SpeedInUnits(350, Units::Metric);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed380kph)] = SpeedInUnits(380, Units::Metric);

  // Mile per hours.
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed3mph)] = SpeedInUnits(3, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed4mph)] = SpeedInUnits(4, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed5mph)] = SpeedInUnits(5, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed6mph)] = SpeedInUnits(6, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed7mph)] = SpeedInUnits(7, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed8mph)] = SpeedInUnits(8, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed9mph)] = SpeedInUnits(9, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed10mph)] = SpeedInUnits(10, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed12mph)] = SpeedInUnits(12, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed13mph)] = SpeedInUnits(13, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed14mph)] = SpeedInUnits(14, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed15mph)] = SpeedInUnits(15, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed17mph)] = SpeedInUnits(17, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed18mph)] = SpeedInUnits(18, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed19mph)] = SpeedInUnits(19, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed20mph)] = SpeedInUnits(20, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed24mph)] = SpeedInUnits(24, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed25mph)] = SpeedInUnits(25, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed30mph)] = SpeedInUnits(30, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed35mph)] = SpeedInUnits(35, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed40mph)] = SpeedInUnits(40, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed45mph)] = SpeedInUnits(45, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed50mph)] = SpeedInUnits(50, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed55mph)] = SpeedInUnits(55, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed59mph)] = SpeedInUnits(59, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed60mph)] = SpeedInUnits(60, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed65mph)] = SpeedInUnits(65, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed70mph)] = SpeedInUnits(70, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed75mph)] = SpeedInUnits(75, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed79mph)] = SpeedInUnits(79, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed80mph)] = SpeedInUnits(80, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed85mph)] = SpeedInUnits(85, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed90mph)] = SpeedInUnits(90, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed95mph)] = SpeedInUnits(95, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed100mph)] = SpeedInUnits(100, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed105mph)] = SpeedInUnits(105, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed110mph)] = SpeedInUnits(110, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed115mph)] = SpeedInUnits(115, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed120mph)] = SpeedInUnits(120, Units::Imperial);
  m_macroToSpeed[static_cast<uint8_t>(Maxspeed::Speed125mph)] = SpeedInUnits(125, Units::Imperial);

  m_speedToMacro.insert(std::make_pair(SpeedInUnits(kInvalidSpeed, Units::Metric), Maxspeed::Undefined));
  for (size_t i = 1; i < std::numeric_limits<uint8_t>::max(); ++i)
  {
    auto const & speed = m_macroToSpeed[i];
    if (!speed.IsValid())
      continue;

    m_speedToMacro.insert(std::make_pair(speed, static_cast<Maxspeed>(i)));
  }
}

SpeedInUnits MaxspeedConverter::MacroToSpeed(Maxspeed macro) const
{
  return m_macroToSpeed[static_cast<uint8_t>(macro)];
}

Maxspeed MaxspeedConverter::SpeedToMacro(SpeedInUnits const & speed) const
{
  auto const it = m_speedToMacro.find(speed);
  if (it == m_speedToMacro.cend())
    return Maxspeed::Undefined;

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

std::string DebugPrint(SpeedInUnits const & speed)
{
  std::ostringstream oss;
  oss << "SpeedInUnits [ m_speed == " << speed.m_speed
      << ", m_units == " << DebugPrint(speed.m_units) << " ]";
  return oss.str();
}

std::string DebugPrint(Maxspeed maxspeed)
{
  std::ostringstream oss;
  oss << "Maxspeed:" << static_cast<int>(maxspeed) << " Decoded:"
      << DebugPrint(GetMaxspeedConverter().MacroToSpeed(maxspeed));
  return oss.str();
}
}  // namespace routing
