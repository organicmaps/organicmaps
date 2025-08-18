#include "editor/ui2oh.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <set>
#include <string>

#include "3party/opening_hours/opening_hours.hpp"

namespace
{
using osmoh::operator""_h;

osmoh::Timespan const kTwentyFourHours = {0_h, 24_h};

editor::ui::OpeningDays MakeOpeningDays(osmoh::Weekdays const & wds)
{
  std::set<osmoh::Weekday> openingDays;
  for (auto const & wd : wds.GetWeekdayRanges())
  {
    if (wd.HasSunday())
      openingDays.insert(osmoh::Weekday::Sunday);
    if (wd.HasMonday())
      openingDays.insert(osmoh::Weekday::Monday);
    if (wd.HasTuesday())
      openingDays.insert(osmoh::Weekday::Tuesday);
    if (wd.HasWednesday())
      openingDays.insert(osmoh::Weekday::Wednesday);
    if (wd.HasThursday())
      openingDays.insert(osmoh::Weekday::Thursday);
    if (wd.HasFriday())
      openingDays.insert(osmoh::Weekday::Friday);
    if (wd.HasSaturday())
      openingDays.insert(osmoh::Weekday::Saturday);
  }
  return openingDays;
}

void SetUpWeekdays(osmoh::Weekdays const & wds, editor::ui::TimeTable & tt)
{
  tt.SetOpeningDays(MakeOpeningDays(wds));
}

void SetUpTimeTable(osmoh::TTimespans spans, editor::ui::TimeTable & tt)
{
  using namespace osmoh;

  // Expand plus: 13:15+ -> 13:15-24:00.
  for (auto & span : spans)
    span.ExpandPlus();

  std::sort(std::begin(spans), std::end(spans), [](Timespan const & a, Timespan const & b)
  {
    auto const start1 = a.GetStart().GetHourMinutes().GetDuration();
    auto const start2 = b.GetStart().GetHourMinutes().GetDuration();

    return start1 < start2;
  });

  // Take first start and last end as opening time span.
  tt.SetOpeningTime({spans.front().GetStart(), spans.back().GetEnd()});

  // Add an end of a span of index i and start of following span
  // as exclude time.
  for (size_t i = 0; i + 1 < spans.size(); ++i)
    tt.AddExcludeTime({spans[i].GetEnd(), spans[i + 1].GetStart()});
}

int32_t WeekdayNumber(osmoh::Weekday const wd)
{
  return static_cast<int32_t>(wd);
}

constexpr uint32_t kDaysInWeek = 7;

// Shifts values from 1 to 7 like this: 1 2 3 4 5 6 7 -> 2 3 4 5 6 7 1
int32_t NextWeekdayNumber(osmoh::Weekday const wd)
{
  auto dayNumber = WeekdayNumber(wd);
  // If first element of the gourp would be evaluated to 0
  // the resulting formula whould be (dayNumber + 1) % kDaysInWeek.
  // Since the first one evaluates to 1
  // the formula is:
  return dayNumber % kDaysInWeek + 1;
}

// Returns a vector of Weekdays with no gaps and with Sunday as the last day.
// Exampls:
// su, mo, we -> mo, we, su;
// su, mo, fr, sa -> fr, sa, su, mo.
std::vector<osmoh::Weekday> RemoveInversion(editor::ui::OpeningDays const & days)
{
  std::vector<osmoh::Weekday> result(begin(days), end(days));
  if ((NextWeekdayNumber(result.back()) != WeekdayNumber(result.front()) && result.back() != osmoh::Weekday::Sunday) ||
      result.size() < 2)
    return result;

  auto inversion = adjacent_find(begin(result), end(result), [](osmoh::Weekday const a, osmoh::Weekday const b)
  { return NextWeekdayNumber(a) != WeekdayNumber(b); });

  if (inversion != end(result))
    rotate(begin(result), ++inversion, end(result));

  if (result.front() == osmoh::Weekday::Sunday)
    rotate(begin(result), begin(result) + 1, end(result));

  return result;
}

using Weekdays = std::vector<osmoh::Weekday>;

std::vector<Weekdays> SplitIntoIntervals(editor::ui::OpeningDays const & days)
{
  ASSERT_GREATER(days.size(), 0, ("At least one day must present."));
  std::vector<Weekdays> result;
  auto const & noInversionDays = RemoveInversion(days);
  ASSERT(!noInversionDays.empty(), ());

  auto previous = *begin(noInversionDays);
  result.push_back({previous});

  for (auto it = next(begin(noInversionDays)); it != end(noInversionDays); ++it)
  {
    if (NextWeekdayNumber(previous) != WeekdayNumber(*it))
      result.push_back({});
    result.back().push_back(*it);
    previous = *it;
  }

  return result;
}

osmoh::Weekdays MakeWeekdays(editor::ui::TimeTable const & tt)
{
  osmoh::Weekdays wds;

  for (auto const & daysInterval : SplitIntoIntervals(tt.GetOpeningDays()))
  {
    osmoh::WeekdayRange wdr;
    wdr.SetStart(*begin(daysInterval));
    if (daysInterval.size() > 1)
      wdr.SetEnd(*(prev(end(daysInterval))));

    wds.AddWeekdayRange(wdr);
  }

  return wds;
}

osmoh::TTimespans MakeTimespans(editor::ui::TimeTable const & tt)
{
  if (tt.IsTwentyFourHours())
    return {kTwentyFourHours};

  auto const & excludeTime = tt.GetExcludeTime();
  if (excludeTime.empty())
    return {tt.GetOpeningTime()};

  osmoh::TTimespans spans{{tt.GetOpeningTime().GetStart(), excludeTime[0].GetStart()}};

  for (size_t i = 0; i + 1 < excludeTime.size(); ++i)
    spans.emplace_back(excludeTime[i].GetEnd(), excludeTime[i + 1].GetStart());

  spans.emplace_back(excludeTime.back().GetEnd(), tt.GetOpeningTime().GetEnd());

  return spans;
}

editor::ui::OpeningDays const kWholeWeek = {
    osmoh::Weekday::Monday, osmoh::Weekday::Tuesday,  osmoh::Weekday::Wednesday, osmoh::Weekday::Thursday,
    osmoh::Weekday::Friday, osmoh::Weekday::Saturday, osmoh::Weekday::Sunday};

editor::ui::OpeningDays GetCommonDays(editor::ui::OpeningDays const & a, editor::ui::OpeningDays const & b)
{
  editor::ui::OpeningDays result;
  std::set_intersection(begin(a), end(a), begin(b), end(b), inserter(result, begin(result)));
  return result;
}

osmoh::HourMinutes::TMinutes::rep GetDuration(osmoh::Time const & time)
{
  return time.GetHourMinutes().GetDurationCount();
}

bool Includes(osmoh::Timespan const & a, osmoh::Timespan const & b)
{
  return GetDuration(a.GetStart()) <= GetDuration(b.GetStart()) && GetDuration(b.GetEnd()) <= GetDuration(a.GetEnd());
}

bool ExcludeRulePart(osmoh::RuleSequence const & rulePart, editor::ui::TimeTableSet & tts)
{
  auto const ttsInitialSize = tts.Size();
  for (size_t i = 0; i < ttsInitialSize; ++i)
  {
    auto tt = tts.Get(i);
    auto const ttOpeningDays = tt.GetOpeningDays();
    auto const commonDays = GetCommonDays(ttOpeningDays, MakeOpeningDays(rulePart.GetWeekdays()));

    auto const removeCommonDays = [&commonDays](editor::ui::TimeTableSet::Proxy & tt)
    {
      for (auto const day : commonDays)
        VERIFY(tt.RemoveWorkingDay(day), ("Can't remove working day"));
      VERIFY(tt.Commit(), ("Can't commit changes"));
    };

    auto const twentyFourHoursGuard = [](editor::ui::TimeTable & tt)
    {
      if (tt.IsTwentyFourHours())
      {
        tt.SetTwentyFourHours(false);
        // TODO(mgsergio): Consider TimeTable refactoring:
        // get rid of separation of TwentyFourHours and OpeningTime.
        tt.SetOpeningTime(kTwentyFourHours);
      }
    };

    auto const & excludeTime = rulePart.GetTimes();
    // The whole rule matches to the tt.
    if (commonDays.size() == ttOpeningDays.size())
    {
      // rulePart applies to commonDays in a whole.
      if (excludeTime.empty())
        return tts.Remove(i);

      twentyFourHoursGuard(tt);

      for (auto const & time : excludeTime)
      {
        // Whatever it is, it's already closed at a time out of opening time.
        if (!Includes(tt.GetOpeningTime(), time))
          continue;

        // The whole opening time interval should be switched off
        if (!tt.AddExcludeTime(time))
          return tts.Remove(i);
      }
      VERIFY(tt.Commit(), ("Can't update time table"));
      return true;
    }
    // A rule is applied to a subset of a time table. We should
    // subtract common parts from tt and add a new time table if needed.
    if (commonDays.size() != 0)
    {
      // rulePart applies to commonDays in a whole.
      if (excludeTime.empty())
      {
        removeCommonDays(tt);
        continue;
      }

      twentyFourHoursGuard(tt);

      editor::ui::TimeTable copy = tt;
      VERIFY(copy.SetOpeningDays(commonDays), ("Can't set opening days"));

      auto doAppendRest = true;
      for (auto const & time : excludeTime)
      {
        // Whatever it is, it's already closed at a time out of opening time.
        if (!Includes(copy.GetOpeningTime(), time))
          continue;

        // The whole opening time interval should be switched off
        if (!copy.AddExcludeTime(time))
        {
          doAppendRest = false;
          break;
        }
      }

      removeCommonDays(tt);

      if (doAppendRest)
        VERIFY(tts.Append(copy), ("Can't add new time table"));
    }
  }
  return true;
}
}  // namespace

namespace editor
{
osmoh::OpeningHours MakeOpeningHours(ui::TimeTableSet const & tts)
{
  ASSERT_GREATER(tts.Size(), 0, ("At least one time table must present."));

  if (tts.IsTwentyFourPerSeven())
  {
    osmoh::RuleSequence rulePart;
    rulePart.SetTwentyFourHours(true);
    return osmoh::OpeningHours({rulePart});
  }

  osmoh::TRuleSequences rule;
  for (auto const & tt : tts)
  {
    osmoh::RuleSequence rulePart;
    rulePart.SetWeekdays(MakeWeekdays(tt));
    rulePart.SetTimes(MakeTimespans(tt));
    rule.push_back(rulePart);
  }

  return rule;
}

bool MakeTimeTableSet(osmoh::OpeningHours const & oh, ui::TimeTableSet & tts)
{
  if (!oh.IsValid())
    return false;

  if (oh.HasYearSelector() || oh.HasWeekSelector() || oh.HasMonthSelector())
    return false;

  tts = ui::TimeTableSet();
  if (oh.IsTwentyFourHours())
    return true;

  bool first = true;
  for (auto const & rulePart : oh.GetRule())
  {
    if (rulePart.IsEmpty())
      continue;

    ui::TimeTable tt = ui::TimeTable::GetUninitializedTimeTable();
    tt.SetOpeningTime(tt.GetPredefinedOpeningTime());

    // Comments and unknown rules belong to advanced mode.
    if (rulePart.GetModifier() == osmoh::RuleSequence::Modifier::Unknown ||
        rulePart.GetModifier() == osmoh::RuleSequence::Modifier::Comment)
      return false;

    if (rulePart.GetModifier() == osmoh::RuleSequence::Modifier::Closed)
    {
      // Off modifier in the first part in oh is useless. Skip it.
      if (first == true)
        continue;

      if (!ExcludeRulePart(rulePart, tts))
        return false;
      continue;
    }

    if (rulePart.HasWeekdays())
      SetUpWeekdays(rulePart.GetWeekdays(), tt);
    else
      tt.SetOpeningDays(kWholeWeek);

    auto const & times = rulePart.GetTimes();

    bool isTwentyFourHours = times.empty() || (times.size() == 1 && times.front() == kTwentyFourHours);

    if (isTwentyFourHours)
    {
      tt.SetTwentyFourHours(true);
    }
    else
    {
      tt.SetTwentyFourHours(false);
      SetUpTimeTable(rulePart.GetTimes(), tt);
    }

    // Check size as well since ExcludeRulePart can add new time tables.
    bool const appended = first && tts.Size() == 1 ? tts.Replace(tt, 0) : tts.Append(tt);
    first = false;
    if (!appended)
      return false;
  }

  // Check if no OH rule has been correctly processed.
  if (first)
  {
    // No OH rule has been correctly processed.
    // Set OH parsing as invalid.
    return false;
  }

  return true;
}
}  // namespace editor
