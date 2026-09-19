#include "testing/testing.hpp"

#include "opening_hours/opening_hours.hpp"

#include "platform/platform_tests_support/helpers.hpp"

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

// GetInfo() with no timezone evaluates in device local time; the helper builds
// the instant with mktime, keeping the test deterministic on any machine.
osmoh::RuleState StateAt(std::string const & oh, int y, int mo, int d, int h, int mi)
{
  auto const month = static_cast<osmoh::MonthDay::Month>(mo);
  return OpeningHours(oh).GetInfo(platform::tests_support::GetUnixtimeByDate(y, month, d, h, mi)).state;
}

// A zone with a fixed offset and no DST transitions; base_offset counts
// 15-minute steps from -16:00 (see om::tz::TimeZone::GetBaseOffset).
om::tz::TimeZone FixedZone(int hoursEastOfUtc)
{
  om::tz::TimeZone tz;
  tz.base_offset = static_cast<uint8_t>(64 + hoursEastOfUtc * 4);
  return tz;
}

// Absolute instant of a local wall-clock hour in a fixed-offset zone.
time_t LocalInstant(int y, unsigned mo, unsigned d, int hour, int hoursEastOfUtc)
{
  using namespace std::chrono;
  auto const days = sys_days{year{y} / month{mo} / day{d}}.time_since_epoch().count();
  return static_cast<time_t>(days * 86400 + (hour - hoursEastOfUtc) * 3600);
}

// Local wall-clock (hour, minute) of an absolute instant in a fixed-offset zone.
std::pair<int, int> LocalHM(time_t t, int hoursEastOfUtc)
{
  int64_t const secondsOfDay = (static_cast<int64_t>(t) + hoursEastOfUtc * 3600) % 86400;
  return {static_cast<int>(secondsOfDay / 3600), static_cast<int>((secondsOfDay % 3600) / 60)};
}
}  // namespace

UNIT_TEST(OpeningHours_Validity)
{
  TEST(OpeningHours("Mo-Fr 08:00-18:00").IsValid(), ());
  TEST(OpeningHours("24/7").IsValid(), ());
  TEST(OpeningHours("Mo-Sa 07:30-21:00; Su,PH off").IsValid(), ());
  // Year selectors, spaced range dashes and full day names are valid input.
  TEST(OpeningHours("2016 Mo-Fr 08:00-10:00").IsValid(), ());
  TEST(OpeningHours("Apr - May 09:00-18:00").IsValid(), ());  // spaced dash
  TEST(OpeningHours("Mon-Fri 10:00-18:00").IsValid(), ());    // full names
  TEST(!OpeningHours("total garbage !!!").IsValid(), ());
}

UNIT_TEST(OpeningHours_RoundTrip)
{
  TEST_EQUAL(ToStr("Mo-Fr 10:00-18:00"), "Mo-Fr 10:00-18:00", ());
  TEST_EQUAL(ToStr("24/7"), "24/7", ());
  // Non-ASCII dash and full day names normalize (issues #3888, #8198).
  TEST_EQUAL(ToStr("Mo-Sa 07:00\xE2\x80\x93"
                   "18:00"),
             "Mo-Sa 07:00-18:00", ());
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
  std::string const oh = "Tu-Fr 11:45-14:30, 19:00-21:45";                   // spaced comma = time list
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 20, 0), osmoh::RuleState::Open, ());    // Tuesday 20:00
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 16, 0), osmoh::RuleState::Closed, ());  // Tuesday, in the break
  TEST_EQUAL(StateAt(oh, 2026, 7, 6, 20, 0), osmoh::RuleState::Closed, ());  // Monday not covered
}

// #7523: extended hours past midnight spill into the next day.
UNIT_TEST(OpeningHours_ExtendedHours)
{
  std::string const oh = "Mo 20:00-26:00";
  TEST_EQUAL(StateAt(oh, 2026, 7, 6, 21, 0), osmoh::RuleState::Open, ());   // Monday 21:00
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 1, 0), osmoh::RuleState::Open, ());    // Tuesday 01:00 (26:00)
  TEST_EQUAL(StateAt(oh, 2026, 7, 7, 3, 0), osmoh::RuleState::Closed, ());  // Tuesday 03:00
}

// closed / off evaluate as closed (issue #1274).
UNIT_TEST(OpeningHours_ClosedValue)
{
  TEST_EQUAL(StateAt("closed", 2026, 7, 6, 12, 0), osmoh::RuleState::Closed, ());
  TEST_EQUAL(StateAt("off", 2026, 7, 6, 12, 0), osmoh::RuleState::Closed, ());
  TEST_EQUAL(StateAt("24/7", 2026, 7, 6, 12, 0), osmoh::RuleState::Open, ());
}

// With a POI coordinate, sun events (sunrise/sunset/dawn/dusk) resolve to the
// real local times instead of the fixed fallback (dawn 06:00, sunrise 07:00,
// sunset 19:00, dusk 20:00).
UNIT_TEST(OpeningHours_SunEvents_RealLocalTimes)
{
  // Golden reference (opening-hours-rs localization tests): Paris (48.87, 2.29)
  // on 2020-06-01 -> sunrise 05:51, sunset 21:46 in local time (CEST, UTC+2).
  ms::LatLon const paris(48.87, 2.29);
  int constexpr kParisJune = 2;  // hours east of UTC
  auto const parisTz = FixedZone(kParisJune);

  // "09:00-sunset" at midday: open, closing at the real sunset (21:46), not 19:00.
  {
    OpeningHours const oh("09:00-sunset");
    time_t const now = LocalInstant(2020, 6, 1, 12, kParisJune);

    auto const withCoord = oh.GetInfo(now, parisTz, paris);
    TEST_EQUAL(withCoord.state, osmoh::RuleState::Open, ());
    auto const [closeH, closeM] = LocalHM(withCoord.nextTimeClosed, kParisJune);
    TEST_EQUAL(closeH, 21, (closeM));
    TEST_EQUAL(closeM, 46, (closeH));

    // Without a coordinate the fixed 19:00 sunset fallback still applies.
    auto const noCoord = oh.GetInfo(now, parisTz);
    auto const [fallbackH, fallbackM] = LocalHM(noCoord.nextTimeClosed, kParisJune);
    TEST_EQUAL(fallbackH, 19, (fallbackM));
    TEST_EQUAL(fallbackM, 0, (fallbackH));

    // The real sunset is strictly later than the fixed fallback.
    TEST_GREATER(withCoord.nextTimeClosed, noCoord.nextTimeClosed, ());
  }

  // "sunrise-sunset" before dawn: closed, opening at the real sunrise (05:51).
  {
    OpeningHours const oh("sunrise-sunset");
    time_t const early = LocalInstant(2020, 6, 1, 4, kParisJune);

    auto const info = oh.GetInfo(early, parisTz, paris);
    TEST_EQUAL(info.state, osmoh::RuleState::Closed, ());
    auto const [openH, openM] = LocalHM(info.nextTimeOpen, kParisJune);
    TEST_EQUAL(openH, 5, (openM));
    TEST_EQUAL(openM, 51, (openH));
  }
}

// Above the polar circle a sun event may not happen at all. That branch is only
// reachable with a coordinate (the fixed fallback always yields a time), and it
// hands control to the evaluator's seasonal substitution.
UNIT_TEST(OpeningHours_SunEvents_PolarDayNight)
{
  ms::LatLon const tromso(69.65, 18.96);
  OpeningHours const oh("sunrise-sunset");

  // Polar day: the sun never sets, so the place stays open at night.
  TEST_EQUAL(oh.GetInfo(LocalInstant(2020, 6, 21, 2, 2), FixedZone(2), tromso).state, osmoh::RuleState::Open, ());
  // Polar night: the sun never rises, so the place stays closed at midday.
  TEST_EQUAL(oh.GetInfo(LocalInstant(2020, 12, 21, 12, 1), FixedZone(1), tromso).state, osmoh::RuleState::Closed, ());
}

// Parse coverage over a real-world OSM corpus ("count|value" per line, copied
// next to the binary by CMake). A regression guard, not a spec check: some
// corpus entries are genuinely invalid, so 100% is neither expected nor wanted.
UNIT_TEST(OpeningHours_YearRangeBeforeMonth)
{
  // Whitespace may separate a year range, list or open end from a month
  // selector.
  TEST(OpeningHours("2020-2030 Jan 10:00-18:00").IsValid(), ());
  TEST(OpeningHours("2020,2022 Dec off").IsValid(), ());
  TEST_EQUAL(StateAt("2020-2030 Jan 10:00-18:00", 2026, 1, 15, 12, 0), osmoh::RuleState::Open, ());
  TEST_EQUAL(StateAt("2020-2030 Jan 10:00-18:00", 2026, 2, 15, 12, 0), osmoh::RuleState::Closed, ());
}

UNIT_TEST(OpeningHours_OpenEndedYearBeforeWeekdays)
{
  // The '+' of an open-ended year selector must not trigger the
  // missing-semicolon repair: one rule, weekends stay closed.
  OpeningHours const oh("1997+ Mo-Fr 10:00-18:00");
  TEST(oh.IsValid(), ());
  TEST_EQUAL(oh.GetRule().size(), 1, ());
  TEST_EQUAL(StateAt("1997+ Mo-Fr 10:00-18:00", 2026, 8, 7, 12, 0), osmoh::RuleState::Open, ());
  TEST_EQUAL(StateAt("1997+ Mo-Fr 10:00-18:00", 2026, 8, 8, 12, 0), osmoh::RuleState::Closed, ());
}

UNIT_TEST(OpeningHours_DuplicateRuleKeepsLast)
{
  // Dedup must drop the earlier copy: the last rule is authoritative under
  // the ';' override semantics, so Monday ends up open 10:00-12:00 again.
  auto const oh = "Mo 10:00-12:00; Mo 14:00-16:00; Mo 10:00-12:00";
  TEST_EQUAL(StateAt(oh, 2026, 8, 3, 10, 30), osmoh::RuleState::Open, ());
  TEST_EQUAL(StateAt(oh, 2026, 8, 3, 14, 30), osmoh::RuleState::Closed, ());
}

UNIT_TEST(OpeningHours_MultiYearRecurrenceHorizon)
{
  using platform::tests_support::GetUnixtimeByDate;
  using Month = osmoh::MonthDay::Month;

  // Recurrences with multi-year gaps must not report "never opens".
  {
    OpeningHours const oh("Feb 29 10:00-11:00");
    TEST(oh.IsValid(), ());
    auto const info = oh.GetInfo(GetUnixtimeByDate(2025, Month::Mar, 1, 12, 0));
    TEST_EQUAL(info.nextTimeOpen, GetUnixtimeByDate(2028, Month::Feb, 29, 10, 0), ());
  }
  {
    OpeningHours const oh("week 53 10:00-11:00");
    TEST(oh.IsValid(), ());
    // The next ISO week 53 after early 2027 starts on 2032-12-27.
    auto const info = oh.GetInfo(GetUnixtimeByDate(2027, Month::Feb, 1, 12, 0));
    TEST_EQUAL(info.nextTimeOpen, GetUnixtimeByDate(2032, Month::Dec, 27, 10, 0), ());
  }
  {
    // Across the skipped century leap year: 2100 is not leap, so the next
    // Feb 29 after 2097 is in 2104 -- 8 years, past the upstream +-4 window.
    OpeningHours const oh("Feb 29 10:00-11:00");
    auto const info = oh.GetInfo(GetUnixtimeByDate(2098, Month::Jun, 15, 12, 0));
    TEST_EQUAL(info.nextTimeOpen, GetUnixtimeByDate(2104, Month::Feb, 29, 10, 0), ());
  }
}

UNIT_TEST(OpeningHours_DegenerateVariableSpan_NoCrash)
{
  // A variable start resolving past a fixed extended end produces an empty
  // span.
  OpeningHours const oh("(sunset+07:00)-25:00");
  TEST(oh.IsValid(), ());
  TEST_EQUAL(StateAt("(sunset+07:00)-25:00", 2026, 8, 4, 12, 0), osmoh::RuleState::Closed, ());
}

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
