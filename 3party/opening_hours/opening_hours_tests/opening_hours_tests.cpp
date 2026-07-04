#include "testing/testing.hpp"

#include "opening_hours.hpp"

#include "base/logging.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace
{
using osmoh::OpeningHours;

std::string ToStr(std::string const & oh)
{
  return ToString(OpeningHours(oh));
}

// Local wall-clock -> time_t. GetInfo() with no timezone evaluates in device
// local time, so building the instant with mktime keeps the test deterministic
// on any machine (mktime and localtime are inverse in the device zone).
time_t MakeTime(int year, int mon, int day, int hour, int min)
{
  std::tm tm{};
  tm.tm_year = year - 1900;
  tm.tm_mon = mon - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_isdst = -1;
  return std::mktime(&tm);
}

osmoh::RuleState StateAt(std::string const & oh, int y, int mo, int d, int h, int mi)
{
  return OpeningHours(oh).GetInfo(MakeTime(y, mo, d, h, mi)).state;
}
}  // namespace

UNIT_TEST(OpeningHours_Validity)
{
  TEST(OpeningHours("Mo-Fr 08:00-18:00").IsValid(), ());
  TEST(OpeningHours("24/7").IsValid(), ());
  TEST(OpeningHours("Mo-Sa 07:30-21:00; Su,PH off").IsValid(), ());
  // The old boost parser rejected these; the port accepts them.
  TEST(OpeningHours("2016 Mo-Fr 08:00-10:00").IsValid(), ());
  TEST(OpeningHours("Apr - May 09:00-18:00").IsValid(), ());        // spaced dash
  TEST(OpeningHours("Mon-Fri 10:00-18:00").IsValid(), ());          // full names
  TEST(!OpeningHours("total garbage !!!").IsValid(), ());
}

UNIT_TEST(OpeningHours_RoundTrip)
{
  TEST_EQUAL(ToStr("Mo-Fr 10:00-18:00"), "Mo-Fr 10:00-18:00", ());
  TEST_EQUAL(ToStr("24/7"), "24/7", ());
  // Non-ASCII dash and full day names normalize (issues #3888, #8198).
  TEST_EQUAL(ToStr("Mo-Sa 07:00\xE2\x80\x93""18:00"), "Mo-Sa 07:00-18:00", ());
  TEST_EQUAL(ToStr("Mon-Fri 10:00-18:00"), "Mo-Fr 10:00-18:00", ());
}

UNIT_TEST(OpeningHours_TwentyFourHours)
{
  // "24/7" is open around the clock; "closed"/"off"/"24/7 closed" are not, and
  // must not report as 24/7 (issues #1274, #12876).
  TEST(OpeningHours("24/7").IsTwentyFourHours(), ());
  TEST(!OpeningHours("closed").IsTwentyFourHours(), ());
  TEST(!OpeningHours("off").IsTwentyFourHours(), ());
  TEST(!OpeningHours("24/7 closed").IsTwentyFourHours(), ());
  TEST(!OpeningHours("24/7 off").IsTwentyFourHours(), ());
}

UNIT_TEST(OpeningHours_Selectors)
{
  TEST(OpeningHours("Mo-Fr 08:00-18:00").HasWeekdaySelector(), ());
  TEST(OpeningHours("2016 Mo-Fr 08:00-10:00").HasYearSelector(), ());
  TEST(OpeningHours("Jan-Mar 08:00-10:00").HasMonthSelector(), ());
  TEST(OpeningHours("week 30 Mo-Fr 08:00-10:00").HasWeekSelector(), ());
}

// #1575 / #8365 / #11509 / #5078: "Su,PH off" must close Sunday, not open it 24h.
UNIT_TEST(OpeningHours_SuPHOff_ClosedOnSunday)
{
  std::string const oh = "Mo-Sa 07:30-21:00; Su,PH off";
  TEST_EQUAL(StateAt(oh, 2026, 7, 5, 12, 0), osmoh::RuleState::Closed, ());  // Sunday
  TEST_EQUAL(StateAt(oh, 2026, 7, 6, 12, 0), osmoh::RuleState::Open, ());    // Monday
  TEST_EQUAL(StateAt(oh, 2026, 7, 6, 6, 0), osmoh::RuleState::Closed, ());   // Monday, before open
}

// #3117: additive rules with a comma unite the ranges.
UNIT_TEST(OpeningHours_AdditiveRule)
{
  std::string const oh = "Tu-Fr 11:45-14:30, 19:00-21:45";  // spaced comma = time list
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 20, 0), osmoh::RuleState::Open, ());    // Tuesday 20:00
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 16, 0), osmoh::RuleState::Closed, ());  // Tuesday, in the break
  TEST_EQUAL(StateAt(oh, 2026, 7, 6, 20, 0), osmoh::RuleState::Closed, ());  // Monday not covered
}

// #7523: extended hours past midnight spill into the next day.
UNIT_TEST(OpeningHours_ExtendedHours)
{
  std::string const oh = "Mo 20:00-26:00";
  TEST_EQUAL(StateAt(oh, 2026, 7, 6, 21, 0), osmoh::RuleState::Open, ());    // Monday 21:00
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 1, 0), osmoh::RuleState::Open, ());     // Tuesday 01:00 (26:00)
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 3, 0), osmoh::RuleState::Closed, ());   // Tuesday 03:00
}

// closed / off evaluate as closed (issue #1274).
UNIT_TEST(OpeningHours_ClosedValue)
{
  TEST_EQUAL(StateAt("closed", 2026, 7, 6, 12, 0), osmoh::RuleState::Closed, ());
  TEST_EQUAL(StateAt("off", 2026, 7, 6, 12, 0), osmoh::RuleState::Closed, ());
  TEST_EQUAL(StateAt("24/7", 2026, 7, 6, 12, 0), osmoh::RuleState::Open, ());
}

// #4186 / #3457: with a POI coordinate, sun events (sunrise/sunset/dawn/dusk)
// resolve to the real local times instead of the fixed fallback (sunset 19:00).
UNIT_TEST(OpeningHours_SunEvents_RealLocalTimes)
{
  using namespace std::chrono;

  // Golden reference (opening-hours-rs localization tests): Paris (48.87, 2.29)
  // on 2020-06-01 -> sunrise 05:51, sunset 21:46 in local time (CEST, UTC+2).
  ms::LatLon const paris(48.87, 2.29);

  // A fixed UTC+2 zone (Paris is on CEST in June): (72 - 64) * 15 == 120 minutes.
  om::tz::TimeZone parisTz;
  parisTz.base_offset = 72;
  int64_t constexpr kOffset = 2 * 3600;  // seconds east of UTC

  int64_t const day = sys_days{2020y / June / 1}.time_since_epoch().count();

  // Local wall-clock (hour, minute) of an absolute instant in the fixed zone.
  auto const localHM = [](time_t t) -> std::pair<int, int>
  {
    int64_t const sod = (static_cast<int64_t>(t) + kOffset) % 86400;
    return {static_cast<int>(sod / 3600), static_cast<int>((sod % 3600) / 60)};
  };

  // "09:00-sunset" at midday: open, closing at the real sunset (21:46), not 19:00.
  {
    OpeningHours const oh("09:00-sunset");
    time_t const now = static_cast<time_t>(day * 86400 + 12 * 3600 - kOffset);  // 12:00 local

    auto const withCoord = oh.GetInfo(now, parisTz, paris);
    TEST_EQUAL(withCoord.state, osmoh::RuleState::Open, ());
    auto const [ch, cm] = localHM(withCoord.nextTimeClosed);
    TEST_EQUAL(ch, 21, (cm));
    TEST_EQUAL(cm, 46, (ch));

    // Without a coordinate the fixed 19:00 sunset fallback still applies.
    auto const noCoord = oh.GetInfo(now, parisTz);
    auto const [fh, fm] = localHM(noCoord.nextTimeClosed);
    TEST_EQUAL(fh, 19, (fm));
    TEST_EQUAL(fm, 0, (fh));

    // The real sunset is strictly later than the fixed fallback.
    TEST_GREATER(withCoord.nextTimeClosed, noCoord.nextTimeClosed, ());
  }

  // "sunrise-sunset" before dawn: closed, opening at the real sunrise (05:51).
  {
    OpeningHours const oh("sunrise-sunset");
    time_t const early = static_cast<time_t>(day * 86400 + 4 * 3600 - kOffset);  // 04:00 local

    auto const info = oh.GetInfo(early, parisTz, paris);
    TEST_EQUAL(info.state, osmoh::RuleState::Closed, ());
    auto const [oh_h, oh_m] = localHM(info.nextTimeOpen);
    TEST_EQUAL(oh_h, 5, (oh_m));
    TEST_EQUAL(oh_m, 51, (oh_h));
  }
}

// Parse coverage over a real-world OSM corpus ("count|value" per line, copied
// next to the binary by CMake). A regression guard, not a spec check: some
// corpus entries are genuinely invalid, so 100% is neither expected nor wanted.
UNIT_TEST(OpeningHours_RealWorldCoverage)
{
  std::ifstream data("opening-count.lst");
  if (!data.is_open())
  {
    LOG(LWARNING, ("opening-count.lst not found; skipping coverage check"));
    return;
  }

  long okWeighted = 0, totalWeighted = 0;
  std::string line;
  while (std::getline(data, line))
  {
    auto const sep = line.find('|');
    if (sep == std::string::npos)
      continue;
    long const count = std::stol(line.substr(0, sep));
    totalWeighted += count;
    if (OpeningHours(line.substr(sep + 1)).IsValid())
      okWeighted += count;
  }

  double const ratio = totalWeighted ? static_cast<double>(okWeighted) / totalWeighted : 1.0;
  LOG(LINFO, ("opening_hours real-world weighted parse coverage:", ratio));
  TEST_GREATER(ratio, 0.94, (okWeighted, "of", totalWeighted));
}
