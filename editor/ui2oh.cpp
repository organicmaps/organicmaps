#include "editor/ui2oh.hpp"

#include "std/algorithm.hpp"
#include "std/array.hpp"
#include "std/string.hpp"

#include "3party/opening_hours/opening_hours.hpp"

namespace
{
void SetUpWeekdays(osmoh::Weekdays const & wds, editor::ui::TimeTable & tt)
{
  set<osmoh::Weekday> workingDays;
  for (auto const & wd : wds.GetWeekdayRanges())
  {
    if (wd.HasSunday())
      workingDays.insert(osmoh::Weekday::Sunday);
    if (wd.HasMonday())
      workingDays.insert(osmoh::Weekday::Monday);
    if (wd.HasTuesday())
      workingDays.insert(osmoh::Weekday::Tuesday);
    if (wd.HasWednesday())
      workingDays.insert(osmoh::Weekday::Wednesday);
    if (wd.HasThursday())
      workingDays.insert(osmoh::Weekday::Thursday);
    if (wd.HasFriday())
      workingDays.insert(osmoh::Weekday::Friday);
    if (wd.HasSaturday())
      workingDays.insert(osmoh::Weekday::Saturday);
  }
  tt.SetWorkingDays(workingDays);
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
}  // namespace

namespace editor
{
osmoh::OpeningHours ConvertOpeningHours(ui::TimeTableSet const & tt)
{
  return string();  // TODO(mgsergio): Implement me.
}

bool ConvertOpeningHours(osmoh::OpeningHours const & oh, ui::TimeTableSet & tts)
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
    ui::TimeTable tt;

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

bool ConvertOpeningHours(string oh, ui::TimeTableSet & tts)
{
  replace(begin(oh), end(oh), '\n', ';');
  return ConvertOpeningHours(osmoh::OpeningHours(oh), tts);
}
} // namespace editor
