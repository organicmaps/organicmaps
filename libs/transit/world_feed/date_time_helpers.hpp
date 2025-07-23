#pragma once

#include <string>
#include <vector>

#include "3party/just_gtfs/just_gtfs.h"
#include "3party/opening_hours/opening_hours.hpp"

namespace transit
{
// Creates osmoh::Time object from GTFS Time |gtfsTime|.
osmoh::Time GetTimeOsmoh(gtfs::Time const & gtfsTime);

// Creates osmoh::RuleSequence with Modifier::Open and osmoh::Timespan with |start| - |end|
// interval.
osmoh::RuleSequence GetRuleSequenceOsmoh(gtfs::Time const & start, gtfs::Time const & end);

// Converts week day |index| in range [0, 6] to the osmoh::Weekday object.
osmoh::Weekday ConvertWeekDayIndexToOsmoh(size_t index);

// Inclusive interval of days and corresponding Open/Closed status.
struct WeekdaysInterval
{
  size_t m_start = 0;
  size_t m_end = 0;
  osmoh::RuleSequence::Modifier m_status = osmoh::RuleSequence::Modifier::DefaultOpen;
};

// Calculates open/closed intervals for |week|.
std::vector<WeekdaysInterval> GetOpenCloseIntervals(std::vector<gtfs::CalendarAvailability> const & week);

// Sets start or end |date| for |range|.
void SetOpeningHoursRange(osmoh::MonthdayRange & range, gtfs::Date const & date, bool isStart);

// Extracts open/closed service days ranges from |serviceDays| to |rules|.
void GetServiceDaysOsmoh(gtfs::CalendarItem const & serviceDays, osmoh::TRuleSequences & rules);

// Extracts open/closed exception service days ranges from |exceptionDays| to |rules|.
void GetServiceDaysExceptionsOsmoh(gtfs::CalendarDates const & exceptionDays, osmoh::TRuleSequences & rules);

// Adds |srcRules| to |dstRules| if they are not present.
void MergeRules(osmoh::TRuleSequences & dstRules, osmoh::TRuleSequences const & srcRules);
}  // namespace transit
