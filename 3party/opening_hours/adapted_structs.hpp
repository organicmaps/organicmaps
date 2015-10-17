#include "osm_time_range.hpp"

#include <boost/fusion/adapted/struct/adapt_struct.hpp>


BOOST_FUSION_ADAPT_STRUCT
(
    osmoh::Time,
    (uint8_t, hours)
    (uint8_t, minutes)
    (uint8_t, flags)
 )

BOOST_FUSION_ADAPT_STRUCT
(
    osmoh::TimeSpan,
    (osmoh::Time, from)
    (osmoh::Time, to)
    (uint8_t, flags)
    (osmoh::Time, period)
 )

BOOST_FUSION_ADAPT_STRUCT
(
    osmoh::Weekday,
    (uint8_t, weekdays)
    (uint16_t, nth)
    (int32_t, offset)
 )

BOOST_FUSION_ADAPT_STRUCT
(
    osmoh::State,
    (uint8_t, state)
    (std::string, comment)
 )

BOOST_FUSION_ADAPT_STRUCT
(
    osmoh::TimeRule,
    (osmoh::TWeekdays, weekdays)
    (osmoh::TTimeSpans, timespan)
    (osmoh::State, state)
    (uint8_t, int_flags)
 )
