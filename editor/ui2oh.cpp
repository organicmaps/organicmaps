#include "editor/ui2oh.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/array.hpp"
#include "std/string.hpp"

#include "3party/opening_hours/opening_hours.hpp"

namespace
{
void SetUpWeekdays(osmoh::Weekdays const & wds, editor::ui::TimeTable & tt)
{
  set<osmoh::Weekday> openingDays;
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
  tt.SetOpeningDays(openingDays);
}

void SetUpTimeTable(osmoh::TTimespans spans, editor::ui::TimeTable & tt)
{
  using namespace osmoh;

  sort(begin(spans), end(spans), [](Timespan const & a, Timespan const & b)
       {
         auto const start1 = a.GetStart().GetHourMinutes().GetDuration();
         auto const start2 = b.GetStart().GetHourMinutes().GetDuration();

         return start1 < start2;
       });

  // Take first start and last end as opening time span.
  tt.SetOpeningTime({spans.front().GetStart(), spans.back().GetEnd()});

  // Add an end of a span of index i and start of following span
  // as exclude time.
  for (auto i = 0; i + 1 < spans.size(); ++i)
    tt.AddExcludeTime({spans[i].GetEnd(), spans[i + 1].GetStart()});
}

int32_t WeekdayNumber(osmoh::Weekday const wd) { return static_cast<int32_t>(wd); }

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
vector<osmoh::Weekday> RemoveInversion(editor::ui::TOpeningDays const & days)
{
  vector<osmoh::Weekday> result(begin(days), end(days));
  if ((NextWeekdayNumber(result.back()) != WeekdayNumber(result.front()) &&
       result.back() != osmoh::Weekday::Sunday) || result.size() < 2)
    return result;

  auto inversion = adjacent_find(begin(result), end(result),
                [](osmoh::Weekday const a, osmoh::Weekday const b)
                {
                  return NextWeekdayNumber(a) != WeekdayNumber(b);
                });

  if (inversion != end(result))
    rotate(begin(result), ++inversion, end(result));

  if (result.front() == osmoh::Weekday::Sunday)
    rotate(begin(result), begin(result) + 1, end(result));

  return result;
}

using TWeekdays = vector<osmoh::Weekday>;

vector<TWeekdays> SplitIntoIntervals(editor::ui::TOpeningDays const & days)
{
  ASSERT_GREATER(days.size(), 0, ("At least one day must present."));
  vector<TWeekdays> result;
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
    return {};

  auto const & excludeTime = tt.GetExcludeTime();
  if (excludeTime.empty())
    return {tt.GetOpeningTime()};

  osmoh::TTimespans spans{{tt.GetOpeningTime().GetStart(), excludeTime[0].GetStart()}};

  for (auto i = 0; i + 1 < excludeTime.size(); ++i)
    spans.emplace_back(excludeTime[i].GetEnd(), excludeTime[i + 1].GetStart());

  spans.emplace_back(excludeTime.back().GetEnd(), tt.GetOpeningTime().GetEnd());

  return spans;
}
}  // namespace

namespace editor
{
osmoh::OpeningHours MakeOpeningHours(ui::TimeTableSet const & tts)
{
  ASSERT_GREATER(tts.Size(), 0, ("At least one time table must present."));
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
    ui::TimeTable tt = ui::TimeTable::GetUninitializedTimeTable();
    tt.SetOpeningTime(tt.GetPredefinedOpeningTime());

    // TODO(mgsergio): We don't handle cases with speciffic time off.
    // I.e. Mo-Fr 08-20; Fr 18:00-17:00 off.
    // Can be implemented later.
    if (rulePart.GetModifier() == osmoh::RuleSequence::Modifier::Closed ||
        rulePart.GetModifier() == osmoh::RuleSequence::Modifier::Unknown ||
        rulePart.GetModifier() == osmoh::RuleSequence::Modifier::Comment)
      continue;

    if (rulePart.HasWeekdays())
      SetUpWeekdays(rulePart.GetWeekdays(), tt);

    if (rulePart.HasTimes())
    {
      tt.SetTwentyFourHours(false);
      SetUpTimeTable(rulePart.GetTimes(), tt);
    }
    else
    {
      tt.SetTwentyFourHours(true);
    }

    bool const appended = first ? tts.Replace(tt, 0) : tts.Append(tt);
    first = false;
    if (!appended)
      return false;
  }

  return true;
}
} // namespace editor
