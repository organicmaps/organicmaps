#include "osm_time_range.hpp"
#include <boost/fusion/adapted/adt/adapt_adt.hpp>


BOOST_FUSION_ADAPT_ADT
(
    osmoh::Time,
    (osmoh::Time::THours, osmoh::Time::THours, obj.GetHours(), obj.SetHours(val))
    (osmoh::Time::TMinutes, osmoh::Time::TMinutes, obj.GetMinutes(), obj.SetMinutes(val))
    (osmoh::Time::EEvent, osmoh::Time::EEvent, obj.GetEvent(), obj.SetEvent(val))
 )

BOOST_FUSION_ADAPT_ADT
(
    osmoh::Timespan,
    (osmoh::Time const &, osmoh::Time const &, obj.GetStart(), obj.SetStart(val))
    (osmoh::Time const &, osmoh::Time const & , obj.GetEnd(), obj.SetEnd(val))
    (osmoh::Time const &, osmoh::Time const &, obj.GetPeriod(), obj.SetPeriod(val))
    (bool, bool, obj.HasPlus(), obj.SetPlus(val))
 )

// BOOST_FUSION_ADAPT_STRUCT
// (
//     osmoh::Weekday,
//     (uint8_t, weekdays)
//     (uint16_t, nth)
//     (int32_t, offset)
//  )

// BOOST_FUSION_ADAPT_STRUCT
// (
//     osmoh::State,
//     (uint8_t, state)
//     (std::string, comment)
//  )

// BOOST_FUSION_ADAPT_STRUCT
// (
//     osmoh::TimeRule,
//     (osmoh::TWeekdays, weekdays)
//     (osmoh::TTimeSpans, timespan)
//     (osmoh::State, state)
//     (uint8_t, int_flags)
//  )
