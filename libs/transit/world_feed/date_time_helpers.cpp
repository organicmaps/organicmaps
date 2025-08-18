#include "transit/world_feed/date_time_helpers.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <cstdint>
#include <iosfwd>
#include <tuple>
#include <utility>

#include <boost/date_time/gregorian/gregorian.hpp>

namespace transit
{
osmoh::Time GetTimeOsmoh(gtfs::Time const & gtfsTime)
{
  uint16_t hh;
  uint16_t mm;
  std::tie(hh, mm, std::ignore) = gtfsTime.get_hh_mm_ss();
  return osmoh::Time(osmoh::Time::THours(hh) + osmoh::Time::TMinutes(mm));
}

osmoh::RuleSequence GetRuleSequenceOsmoh(gtfs::Time const & start, gtfs::Time const & end)
{
  osmoh::RuleSequence ruleSeq;
  ruleSeq.SetModifier(osmoh::RuleSequence::Modifier::Open);
  osmoh::Timespan range(GetTimeOsmoh(start), GetTimeOsmoh(end));
  ruleSeq.SetTimes({range});
  return ruleSeq;
}

osmoh::MonthdayRange GetMonthdayRangeFromDates(gtfs::Date const & start, gtfs::Date const & end)
{
  osmoh::MonthdayRange range;
  SetOpeningHoursRange(range, start, true /* isStart */);
  SetOpeningHoursRange(range, end, false /* isStart */);
  return range;
}

struct AccumExceptionDates
{
public:
  using GregorianInterval = std::pair<boost::gregorian::date, boost::gregorian::date>;
  using GtfsInterval = std::pair<gtfs::Date, gtfs::Date>;

  void InitIntervals(boost::gregorian::date const & gregorianDate, gtfs::Date const & gtfsDate);
  void AddRange();
  bool IsInited() const;

  GregorianInterval m_GregorianInterval;
  GtfsInterval m_GtfsInterval;
  osmoh::TMonthdayRanges m_ranges;

private:
  bool m_inited = false;
};

void AccumExceptionDates::InitIntervals(boost::gregorian::date const & gregorianDate, gtfs::Date const & gtfsDate)
{
  m_GregorianInterval = std::make_pair(gregorianDate, gregorianDate);
  m_GtfsInterval = std::make_pair(gtfsDate, gtfsDate);
  m_inited = true;
}

void AccumExceptionDates::AddRange()
{
  osmoh::MonthdayRange range = GetMonthdayRangeFromDates(m_GtfsInterval.first, m_GtfsInterval.second);
  m_ranges.push_back(range);
  m_inited = false;
}

bool AccumExceptionDates::IsInited() const
{
  return m_inited;
}

osmoh::Weekday ConvertWeekDayIndexToOsmoh(size_t index)
{
  // Monday index in osmoh is 2.
  index += 2;

  if (index == 7)
    return osmoh::Weekday::Saturday;
  if (index == 8)
    return osmoh::Weekday::Sunday;

  return osmoh::ToWeekday(index);
}

std::vector<WeekdaysInterval> GetOpenCloseIntervals(std::vector<gtfs::CalendarAvailability> const & week)
{
  std::vector<WeekdaysInterval> intervals;

  WeekdaysInterval interval;
  for (size_t i = 0; i < week.size(); ++i)
  {
    osmoh::RuleSequence::Modifier const status = week[i] == gtfs::CalendarAvailability::Available
                                                   ? osmoh::RuleSequence::Modifier::DefaultOpen
                                                   : osmoh::RuleSequence::Modifier::Closed;
    if (status == interval.m_status)
    {
      interval.m_end = i;
    }
    else
    {
      if (i > 0)
        intervals.push_back(interval);
      interval.m_start = i;
      interval.m_end = i;
      interval.m_status = status;
    }
    if (i == week.size() - 1)
      intervals.push_back(interval);
  }

  return intervals;
}

void SetOpeningHoursRange(osmoh::MonthdayRange & range, gtfs::Date const & date, bool isStart)
{
  if (!date.is_provided())
  {
    LOG(LINFO, ("Date is not provided in the calendar."));
    return;
  }

  auto const & [year, month, day] = date.get_yyyy_mm_dd();

  osmoh::MonthDay monthDay;
  monthDay.SetYear(year);
  monthDay.SetMonth(static_cast<osmoh::MonthDay::Month>(month));
  monthDay.SetDayNum(day);

  if (isStart)
    range.SetStart(monthDay);
  else
    range.SetEnd(monthDay);
}

void GetServiceDaysOsmoh(gtfs::CalendarItem const & serviceDays, osmoh::TRuleSequences & rules)
{
  osmoh::MonthdayRange range = GetMonthdayRangeFromDates(serviceDays.start_date, serviceDays.end_date);
  osmoh::TMonthdayRanges const rangesMonths{range};

  std::vector<gtfs::CalendarAvailability> const weekDayStatuses = {
      serviceDays.monday, serviceDays.tuesday,  serviceDays.wednesday, serviceDays.thursday,
      serviceDays.friday, serviceDays.saturday, serviceDays.sunday};

  auto const & intervals = GetOpenCloseIntervals(weekDayStatuses);

  osmoh::RuleSequence ruleSeqOpen;
  osmoh::RuleSequence ruleSeqClose;

  for (auto const & interval : intervals)
  {
    osmoh::RuleSequence & ruleSeq =
        interval.m_status == osmoh::RuleSequence::Modifier::DefaultOpen ? ruleSeqOpen : ruleSeqClose;
    ruleSeq.SetMonths(rangesMonths);
    ruleSeq.SetModifier(interval.m_status);

    osmoh::WeekdayRange weekDayRange;
    weekDayRange.SetStart(ConvertWeekDayIndexToOsmoh(interval.m_start));
    weekDayRange.SetEnd(ConvertWeekDayIndexToOsmoh(interval.m_end));

    osmoh::TWeekdayRanges weekDayRanges;
    weekDayRanges.push_back(weekDayRange);

    osmoh::Weekdays weekDays;
    weekDays.SetWeekdayRanges(weekDayRanges);
    ruleSeq.SetWeekdays(weekDays);
  }

  if (ruleSeqOpen.HasWeekdays())
    rules.push_back(ruleSeqOpen);

  if (ruleSeqClose.HasWeekdays())
    rules.push_back(ruleSeqClose);
}

void AppendMonthRules(osmoh::RuleSequence::Modifier const & status, osmoh::TMonthdayRanges const & monthRanges,
                      osmoh::TRuleSequences & rules)
{
  osmoh::RuleSequence ruleSeq;
  ruleSeq.SetMonths(monthRanges);
  ruleSeq.SetModifier(status);
  rules.push_back(ruleSeq);
}

void GetServiceDaysExceptionsOsmoh(gtfs::CalendarDates const & exceptionDays, osmoh::TRuleSequences & rules)
{
  if (exceptionDays.empty())
    return;

  AccumExceptionDates accumOpen;
  AccumExceptionDates accumClosed;

  for (size_t i = 0; i < exceptionDays.size(); ++i)
  {
    AccumExceptionDates & curAccum =
        (exceptionDays[i].exception_type == gtfs::CalendarDateException::Added) ? accumOpen : accumClosed;

    auto const [year, month, day] = exceptionDays[i].date.get_yyyy_mm_dd();
    boost::gregorian::date const date{year, month, day};
    if (!curAccum.IsInited())
    {
      curAccum.InitIntervals(date, exceptionDays[i].date);
    }
    else
    {
      auto & prevDate = curAccum.m_GregorianInterval.second;
      boost::gregorian::date_duration duration = date - prevDate;
      CHECK(!duration.is_negative(), ());

      if (duration.days() == 1)
      {
        prevDate = date;
        curAccum.m_GtfsInterval.second = exceptionDays[i].date;
      }
      else
      {
        curAccum.AddRange();
        curAccum.InitIntervals(date, exceptionDays[i].date);
      }
    }

    AccumExceptionDates & prevAccum =
        (exceptionDays[i].exception_type == gtfs::CalendarDateException::Added) ? accumClosed : accumOpen;
    if (prevAccum.IsInited())
      prevAccum.AddRange();

    if (i == exceptionDays.size() - 1)
      curAccum.AddRange();
  }

  if (!accumOpen.m_ranges.empty())
    AppendMonthRules(osmoh::RuleSequence::Modifier::DefaultOpen, accumOpen.m_ranges, rules);

  if (!accumClosed.m_ranges.empty())
    AppendMonthRules(osmoh::RuleSequence::Modifier::Closed, accumClosed.m_ranges, rules);
}

void MergeRules(osmoh::TRuleSequences & dstRules, osmoh::TRuleSequences const & srcRules)
{
  for (auto const & rule : srcRules)
    if (!base::IsExist(dstRules, rule))
      dstRules.push_back(rule);
}
}  // namespace transit
