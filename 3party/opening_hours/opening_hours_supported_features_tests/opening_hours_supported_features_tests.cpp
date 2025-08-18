#include "parse_opening_hours.hpp"
#include "rules_evaluation.hpp"

#define BOOST_TEST_MODULE OpeningHoursSupportedFeatures

#include <ctime>
#include <boost/test/included/unit_test.hpp>

namespace
{
typedef std::tuple<long, long> LongTimeRange;
int countTests = 0;

LongTimeRange RangeToLong(std::string const & start, std::string const & end)
{
  std::tm when{};

  auto const fmt = "%Y.%m.%d %R";

  strptime(start.data(), fmt, &when);
  auto const r1 = mktime(&when);

  strptime(end.data(), fmt, &when);
  return std::make_tuple(r1, mktime(&when));
}


void TestRanges(std::string const & name, std::initializer_list<std::string> const & strings,
                std::string const & rangeStart, std::string const & rangeEnd,
                std::initializer_list<std::vector<std::string>> const & ranges)
{
  for (std::string const & input : strings)
  {
    countTests++;
    osmoh::TRuleSequences rule;
    bool const parsed = Parse(input, rule);
    BOOST_CHECK_MESSAGE(parsed, name << " [" << input << "] not valid");
    if (!parsed)
      continue;

    LongTimeRange ltr = RangeToLong(rangeStart, rangeEnd);
    for (std::vector<std::string> const & range : ranges)
    {
      LongTimeRange lRange = RangeToLong(range[0], range[1]);
      bool failed = false;
      bool checkOpen = range.size() < 4;

      if (!checkOpen)
        std::cout << "Checking unknown " << input << std::endl;
      if (std::get<0>(lRange) - 60 >= std::get<0>(ltr) &&
          !IsClosed(rule, std::get<0>(lRange) - 60))
      {
        failed = true;
        BOOST_CHECK_MESSAGE(false, name << " [" << input << "] not closed before " << range[0]);
      }
      else if (std::get<0>(lRange) + 60 <= std::get<1>(ltr)
               && (checkOpen
                   ? !IsOpen(rule, std::get<0>(lRange) + 60)
                   : !IsUnknown(rule, std::get<0>(lRange) + 60)))
      {
        failed = true;
        BOOST_CHECK_MESSAGE(false, name << " [" << input << "] not " << (checkOpen ? "open" : "unknown") << " after " << range[0]);
      }
      // else if (range.size() > 2 &&
      //          std::get<0>(lRange) + 60 <= std::get<1>(ltr) // &&
      //          // oh(std::get<0>(lRange) + 60).Comment() != range[2]
      //          )
      // {
      //   failed = true;
      //   // BOOST_CHECK_MESSAGE(false, name << " [" << input << "] after " << range[0] << " comment is [" << oh(std::get<0>(lRange) + 60).Comment() << "] not [" << range[2] << "]");
      //   BOOST_CHECK_MESSAGE(false, name << " [" << input << "] after " << range[0] << " comment is [" << "] not [" << range[2] << "]");
      // }
      else if (std::get<1>(lRange) - 60 >= std::get<0>(ltr) &&
               (checkOpen
                ? !IsOpen(rule, std::get<1>(lRange) - 60)
                : !IsUnknown(rule, std::get<1>(lRange) - 60)))
      {
        failed = true;
        BOOST_CHECK_MESSAGE(false, name << " [" << input << "] not " << (checkOpen ? "open" : "unknown") << " before " << range[1]);
      }
      else if (std::get<1>(lRange) + 60 <= std::get<1>(ltr) &&
               !IsClosed(rule, std::get<1>(lRange) + 60))
      {
        failed = true;
        BOOST_CHECK_MESSAGE(false, name << " [" << input << "] not closed after " << range[1]);
      }

      if (failed)
        break;
    }
  }
}

void TestShouldFail(std::string const & name, std::initializer_list<std::string> const & strings)
{
  for (std::string const & input : strings)
  {
    countTests++;
    osmoh::TRuleSequences rule;
    bool const parsed  = Parse(input, rule);
    BOOST_CHECK_MESSAGE(!parsed, name << " [" << input << "] is valid");
  }
}
}  // namespace

BOOST_AUTO_TEST_CASE(OpeningHours_TestJS)
{
  std::string sane_value_suffix("; 00:23-00:42 closed \"warning at correct position?\"");
  // Suffix to add to values to make the value more complex and to spot problems
  // easier without changing there meaning (in most cases).
  std::string value_suffix("; 00:23-00:42 unknown \"warning at correct position?\"");
  // This suffix value is there to test if the warning marks the correct position of the problem.
  std::string value_suffix_to_disable_time_not_used = " 12:00-15:00";
  std::string open_end_comment = "Specified as open end. Closing time was guessed.";

  // time ranges {{{
  TestRanges("Time intervals", {
    "10:00-12:00",
    "08:00-09:00; 10:00-12:00",
    "10:00-12:00,",
    "10:00-12:00;",
    "10-12", // Do not use. Returns warning.
    "10:00-11:00,11:00-12:00",
    "10:00-12:00,10:30-11:30",
    "10:00-14:00; 12:00-14:00 off",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 12:00" },
    { "2012.10.02 10:00", "2012.10.02 12:00" },
    { "2012.10.03 10:00", "2012.10.03 12:00" },
    { "2012.10.04 10:00", "2012.10.04 12:00" },
    { "2012.10.05 10:00", "2012.10.05 12:00" },
    { "2012.10.06 10:00", "2012.10.06 12:00" },
    { "2012.10.07 10:00", "2012.10.07 12:00" },
  });

  TestRanges("Time intervals", {
    "24/7; Mo 15:00-16:00 off", // throws a warning, use next value which is equal.
    "open; Mo 15:00-16:00 off",
    "00:00-24:00; Mo 15:00-16:00 off",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 15:00" },
    { "2012.10.01 16:00", "2012.10.08 00:00" },
  });

  TestRanges("Time zero intervals (always closed)", {
    "off",
    "closed",
    "off; closed",
    "24/7 closed \"always closed\"", // Used on the demo page.
    "24/7: closed \"always closed\"",
    "24/7 closed: \"always closed\"",
    "24/7: closed: \"always closed\"",
    "closed \"always closed\"",
    "off \"always closed\"",
    "00:00-24:00 closed",
    "24/7 closed",
  }, "2012.10.01 0:00", "2018.10.08 0:00", {
  });

  // error tolerance {{{
  TestRanges("Error tolerance: dot as time separator", {
    "10:00-12:00", // reference value for prettify
    "10.00-12.00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 12:00" },
    { "2012.10.02 10:00", "2012.10.02 12:00" },
    { "2012.10.03 10:00", "2012.10.03 12:00" },
    { "2012.10.04 10:00", "2012.10.04 12:00" },
    { "2012.10.05 10:00", "2012.10.05 12:00" },
    { "2012.10.06 10:00", "2012.10.06 12:00" },
    { "2012.10.07 10:00", "2012.10.07 12:00" },
  });

  TestRanges("Error tolerance: dot as time separator", {
    "10:00-14:00; 12:00-14:00 off", // reference value for prettify
    "10-14; 12-14 off", // "22-2", // Do not use. Returns warning.
    "10.00-14.00; 12.00-14.00 off",
    // "10.00-12.00;10.30-11.30",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 12:00" },
    { "2012.10.02 10:00", "2012.10.02 12:00" },
    { "2012.10.03 10:00", "2012.10.03 12:00" },
    { "2012.10.04 10:00", "2012.10.04 12:00" },
    { "2012.10.05 10:00", "2012.10.05 12:00" },
    { "2012.10.06 10:00", "2012.10.06 12:00" },
    { "2012.10.07 10:00", "2012.10.07 12:00" },
  });

  TestRanges("Error tolerance: Correctly handle pm time.", {
    "10:00-12:00,13:00-20:00",       // reference value for prettify
    "10-12,13-20",
    "10am-12am,1pm-8pm",
    "10:00am-12:00am,1:00pm-8:00pm",
    "10:00am-12:00am,1.00pm-8.00pm",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 10:00", "2012.10.01 12:00" },
    { "2012.10.01 13:00", "2012.10.01 20:00" },
    { "2012.10.02 10:00", "2012.10.02 12:00" },
    { "2012.10.02 13:00", "2012.10.02 20:00" },
  });

  TestRanges("Error tolerance: Correctly handle pm time.", {
    "13:00-20:00,10:00-12:00",       // reference value for prettify
    "1pm-8pm,10am-12am",
    // "1pm-8pm/10am-12am", // Can not be corrected as / is a valid token
    "1:00pm-8:00pm,10:00am-12:00am",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 10:00", "2012.10.01 12:00" },
    { "2012.10.01 13:00", "2012.10.01 20:00" },
    { "2012.10.02 10:00", "2012.10.02 12:00" },
    { "2012.10.02 13:00", "2012.10.02 20:00" },
  });

  TestRanges("Error tolerance: Time intervals, short time", {
    "Mo 07:00-18:00", //reference value for prettify
    "Montags 07:00-18:00", //reference value for prettify
    "Mo 7-18", // throws a warning, use previous value which is equal.
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 07:00", "2012.10.01 18:00" },
  });

  TestRanges("Error tolerance: Time range", {
    "Mo 12:00-14:00", // reference value for prettify
    "Mo12:00-14:00",
    "Mo 12:00→14:00",
    "Mo 12:00–14:00",
    "Mo 12:00−14:00",
    "Mo 12:00—14:00",
    "Mo 12:00ー14:00",
    "Mo 12:00=14:00",
    "Mo 12:00 to 14:00",
    "Mo 12:00 до 14:00",
    "Mo 12:00 a 14:00",
    "Mo 12:00 as 14:00",
    "Mo 12:00 á 14:00",
    "Mo 12:00 ás 14:00",
    "Mo 12:00 à 14:00",
    "Mo 12:00 às 14:00",
    "Mo 12:00 ate 14:00",
    "Mo 12:00 till 14:00",
    "Mo 12:00 til 14:00",
    "Mo 12:00 until 14:00",
    "Mo 12:00 through 14:00",
    "Mo 12:00~14:00",
    "Mo 12:00～14:00",
    "Mo 12:00-14：00",
    "Mo 12°°-14:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 12:00", "2012.10.01 14:00" },
  });
  // }}}

  // time range spanning midnight {{{
  TestRanges("Time ranges spanning midnight", {
    "22:00-02:00",
    "22:00-26:00",
    "22-2", // Do not use. Returns warning.
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 02:00" },
    { "2012.10.01 22:00", "2012.10.02 02:00" },
    { "2012.10.02 22:00", "2012.10.03 02:00" },
    { "2012.10.03 22:00", "2012.10.04 02:00" },
    { "2012.10.04 22:00", "2012.10.05 02:00" },
    { "2012.10.05 22:00", "2012.10.06 02:00" },
    { "2012.10.06 22:00", "2012.10.07 02:00" },
    { "2012.10.07 22:00", "2012.10.08 00:00" },
  });

  TestRanges("Time ranges spanning midnight", {
    "22:00-26:00", // reference value for prettify
    "22-26", // Do not use. Returns warning.
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 02:00" },
    { "2012.10.01 22:00", "2012.10.02 02:00" },
    { "2012.10.02 22:00", "2012.10.03 02:00" },
    { "2012.10.03 22:00", "2012.10.04 02:00" },
    { "2012.10.04 22:00", "2012.10.05 02:00" },
    { "2012.10.05 22:00", "2012.10.06 02:00" },
    { "2012.10.06 22:00", "2012.10.07 02:00" },
    { "2012.10.07 22:00", "2012.10.08 00:00" },
  });

  TestRanges("Time ranges spanning midnight", {
    "We 22:00-22:00",
    "We22:00-22:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.03 22:00", "2012.10.04 22:00" },
  });

  TestRanges("Time ranges spanning midnight with date overwriting", {
    "22:00-02:00; Tu 12:00-14:00",
    "22:00-02:00; Tu12:00-14:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 02:00" },
    { "2012.10.01 22:00", "2012.10.02 00:00" },
    { "2012.10.02 12:00", "2012.10.02 14:00" },
    { "2012.10.03 00:00", "2012.10.03 02:00" },
    { "2012.10.03 22:00", "2012.10.04 02:00" },
    { "2012.10.04 22:00", "2012.10.05 02:00" },
    { "2012.10.05 22:00", "2012.10.06 02:00" },
    { "2012.10.06 22:00", "2012.10.07 02:00" },
    { "2012.10.07 22:00", "2012.10.08 00:00" },
  });

  TestRanges("Time ranges spanning midnight with date overwriting (complex real world example)", {
    "Su-Tu 11:00-01:00, We-Th 11:00-03:00, Fr 11:00-06:00, Sa 11:00-07:00",
    "Su-Tu 11:00-01:00, We-Th11:00-03:00, Fr 11:00-06:00, Sa 11:00-07:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 01:00", }, // Mo: Su-Tu 11:00-01:00
    { "2012.10.01 11:00", "2012.10.02 01:00", }, // Mo: Su-Tu 11:00-01:00
    { "2012.10.02 11:00", "2012.10.03 01:00", }, // Tu: Su-Tu 11:00-01:00
    { "2012.10.03 11:00", "2012.10.04 03:00", }, // We: We-Th 11:00-03:00
    { "2012.10.04 11:00", "2012.10.05 03:00", }, // Th: We-Th 11:00-03:00
    { "2012.10.05 11:00", "2012.10.06 06:00", }, // Fr: Fr 11:00-06:00
    { "2012.10.06 11:00", "2012.10.07 07:00", }, // Sa: Sa 11:00-07:00
    { "2012.10.07 11:00", "2012.10.08 00:00", }, // Su: Su-Tu 11:00-01:00
  });

  TestRanges("Time ranges spanning midnight (maximum supported)", {
    "Tu 23:59-48:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.02 23:59", "2012.10.04 00:00" },
  });

  TestRanges("Time ranges spanning midnight with open ened (maximum supported)", {
    "Tu 23:59-40:00+",
    // "Tu 23:59-00:00 open, 24:00-40:00 open, 40:00+ open, 40:00+",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.02 23:59", "2012.10.03 16:00" },
    { "2012.10.03 16:00", "2012.10.04 00:00", open_end_comment, "true" },
  });
  // }}}

  // }}}

  // open end {{{
  TestRanges("Open end", {
    "07:00+ open \"visit there website to know if they did already close\"", // specified comments should not be overridden
    "07:00+ unknown \"visit there website to know if they did already close\"", // will always interpreted as unknown
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 07:00", "2012.10.02 00:00", "visit there website to know if they did already close", "true" },
  });

  TestRanges("Open end", {
    "17:00+",
    "17:00-late",
    "17:00 til late",
    "17:00 till late",
    "17:00 bis Open End",
    "17:00-open end",
    // "17:00 – Open End", // "–" matches first.
    "17:00-openend",
    "17:00+; 15:00-16:00 off",
    "15:00-16:00 off; 17:00+",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 00:00", "2012.10.01 03:00", open_end_comment, "true" },
    { "2012.10.01 17:00", "2012.10.02 00:00", open_end_comment, "true" },
  });

  TestRanges("Open end, variable time", {
    "sunrise+",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 07:22", "2012.10.02 00:00", open_end_comment, "true" },
  });

  TestRanges("Open end, variable time", {
    "(sunrise+01:00)+",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 08:22", "2012.10.02 00:00", open_end_comment, "true" },
  });

  TestRanges("Open end", {
    "17:00+ off",
    "17:00+off",
    "17:00-19:00 off",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
  });

  TestRanges("Open end", {
    // "12:00-16:00,07:00+", // Fails. This is ok. Just put your time selectors in the correct order.
    "07:00+,12:00-16:00",
    "07:00+,12:00-13:00,13:00-16:00",
    "07:00+,12:00-16:00; 16:00-24:00 closed \"needed because of open end\"", // Now obsolete: https://github.com/ypid/opening_hours.js/issues/48
  }, "2012.10.01 0:00", "2012.10.02 5:00", {
    { "2012.10.01 07:00", "2012.10.01 12:00", open_end_comment, "true" },
    { "2012.10.01 12:00", "2012.10.01 16:00" },
  });

  TestRanges("Open end", {
    "05:00-06:00,06:45-07:00+,13:00-16:00",
    "06:45-07:00+,05:00-06:00,13:00-16:00",
    "06:45-07:00+,05:00-06:00,13:00-14:00,14:00-16:00",
  }, "2012.10.01 0:00", "2012.10.02 5:00", {
    { "2012.10.01 05:00", "2012.10.01 06:00" },
    { "2012.10.01 06:45", "2012.10.01 07:00" },
    { "2012.10.01 07:00", "2012.10.01 13:00", open_end_comment, "true" },
    { "2012.10.01 13:00", "2012.10.01 16:00" },
  });

  /* To complicated, just don‘t use them … {{{ */
  TestRanges("Open end", {
    "17:00+,13:00-02:00; 02:00-03:00 closed \"needed because of open end\"",
    "17:00+,13:00-02:00; 02:00-03:00 closed \"needed because of open end\"",
    // "17:00-00:00 unknown open_end_comment, "true", 13:00-00:00 open" // First internal rule.
    // + ", " {> overwritten part: 00:00-03:00 open" <} + "00:00-02:00 open", // Second internal rule.
  }, "2012.10.01 0:00", "2012.10.02 5:00", {
    { "2012.10.01 00:00", "2012.10.01 02:00" },
    { "2012.10.01 13:00", "2012.10.02 02:00" },
  });

  TestRanges("Open end", {
    "13:00-17:00+", // Use this.
    "13:00-17:00,17:00+",
    "13:00-02:00,17:00+", // Do not use.
    "13:00-17:00 open, 17:00+"
  }, "2012.10.01 0:00", "2012.10.02 5:00", {
    { "2012.10.01 00:00", "2012.10.01 03:00", open_end_comment, "true" },
    { "2012.10.01 13:00", "2012.10.01 17:00" },
    { "2012.10.01 17:00", "2012.10.02 03:00", open_end_comment, "true" },
  });

  TestRanges("Open end", {
    // "05:00-06:00,17:00+,13:00-02:00",
    // "05:00-06:00,13:00-02:00,17:00+",
  }, "2012.10.01 0:00", "2012.10.02 5:00", {
    { "2012.10.01 00:00", "2012.10.01 02:00" },
    { "2012.10.01 05:00", "2012.10.01 06:00" },
    { "2012.10.01 13:00", "2012.10.02 02:00" },
  });
  /* }}} */

  // proposal: opening hours open end fixed time extension {{{
  // https://wiki.openstreetmap.org/wiki/Proposed_features/opening_hours_open_end_fixed_time_extension

  TestRanges("Fixed time followed by open end", {
    "14:00-17:00+",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 00:00", "2012.10.01 03:00", open_end_comment, "true" },
    { "2012.10.01 14:00", "2012.10.01 17:00" },
    { "2012.10.01 17:00", "2012.10.02 00:00", open_end_comment, "true" },
  });

  TestRanges("Fixed time followed by open end, wrapping over midnight", {
    "Mo 22:00-04:00+",
    "Mo 22:00-28:00+",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 22:00", "2012.10.02 04:00" },
    { "2012.10.02 04:00", "2012.10.02 12:00", open_end_comment, "true" },
  });

  TestRanges("variable time range followed by open end", {
    "14:00-sunset+",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 00:00", "2012.10.01 04:00", open_end_comment, "true" },
    { "2012.10.01 14:00", "2012.10.01 19:00" },
    { "2012.10.01 19:00", "2012.10.02 00:00", open_end_comment, "true" },
  });

  TestRanges("variable time range followed by open end", {
    "sunrise-14:00+",
    "sunrise-14:00,14:00+", // Internally represented as two time selectors.
    "sunrise-14:00 open, 14:00+",
  }, "2012.10.01 0:00", "2012.10.02 5:00", {
    { "2012.10.01 07:22", "2012.10.01 14:00" },
    { "2012.10.01 14:00", "2012.10.02 00:00", open_end_comment, "true" },
  });

  TestRanges("variable time range followed by open end", {
    "sunrise-(sunset+01:00)+",
    "sunrise-(sunset+01:00)+; Su off",
  }, "2012.10.06 0:00", "2012.10.07 0:00", {
    { "2012.10.06 00:00", "2012.10.06 05:00", open_end_comment, "true" },
    { "2012.10.06 07:29", "2012.10.06 19:50" },
    { "2012.10.06 19:50", "2012.10.07 00:00", open_end_comment, "true" },
  });

  TestRanges("variable time range followed by open end, day wrap and different states", {
    "Fr 11:00-24:00+ open \"geöffnet täglich von 11:00 Uhr bis tief in die Nacht\"",
    "Fr 11:00-24:00+ open\"geöffnet täglich von 11:00 Uhr bis tief in die Nacht\"",
    "Fr 11:00-24:00+open \"geöffnet täglich von 11:00 Uhr bis tief in die Nacht\"",
    "Fr 11:00-24:00+open\"geöffnet täglich von 11:00 Uhr bis tief in die Nacht\"",
    "Fr11:00-24:00+open\"geöffnet täglich von 11:00 Uhr bis tief in die Nacht\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.05 11:00", "2012.10.06 00:00", "geöffnet täglich von 11:00 Uhr bis tief in die Nacht" },
    { "2012.10.06 00:00", "2012.10.06 08:00", "geöffnet täglich von 11:00 Uhr bis tief in die Nacht", "true" },
  });
  // }}}
  // }}}

  // variable times {{{
  TestRanges("Variable times e.g. dawn, dusk", {
    "Mo dawn-dusk",
    "dawn-dusk",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 06:50", "2012.10.01 19:32" },
  });

  TestRanges("Variable times e.g. sunrise, sunset", {
    "Mo sunrise-sunset",
    "sunrise-sunset",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 07:22", "2012.10.01 19:00" },
  });

  TestRanges("Variable times e.g. sunrise, sunset without coordinates (→ constant times)", {
    "sunrise-sunset",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 06:00", "2012.10.01 18:00" },
    { "2012.10.02 06:00", "2012.10.02 18:00" },
  });

  TestRanges("Variable times e.g. sunrise, sunset", {
    "sunrise-sunset open \"Beware of sunburn!\"",
    // "sunrise-sunset closed "Beware of sunburn!"", // Not so intuitive I guess.
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 07:22", "2012.10.01 19:00", "Beware of sunburn!" },
  });

  TestRanges("Variable times calculation without coordinates", {
    "(sunrise+01:02)-(sunset-00:30)",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 07:02", "2012.10.01 17:30" },
    { "2012.10.02 07:02", "2012.10.02 17:30" },
  });

  TestRanges("Variable times e.g. dawn, dusk without coordinates (→ constant times)", {
    "dawn-dusk",
    "(dawn+00:00)-dusk", // testing variable time calculation, should not change time
    "dawn-(dusk-00:00)",
    "(dawn+00:00)-(dusk-00:00)",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 05:30", "2012.10.01 18:30" },
    { "2012.10.02 05:30", "2012.10.02 18:30" },
  });

  // weekdays {{{
  TestRanges("Weekdays", {
    "Mo,Th,Sa,Su 10:00-12:00",
    "Mo,Th,weekend 10:00-12:00",        // Throws a warning.
    "Mo & Th and weekends 10:00-12:00", // Throws a warning.
    "Mo,Th,Sa,Su 10:00-12:00",          // Throws a warning.
    "Mo,Th,Sa-Su 10:00-12:00",
    "Th,Sa-Mo 10:00-12:00",
    "10:00-12:00; Tu-We 00:00-24:00 off; Fr 00:00-24:00 off",
    "10:00-12:00; Tu-We off; Fr off",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 12:00" },
    { "2012.10.04 10:00", "2012.10.04 12:00" },
    { "2012.10.06 10:00", "2012.10.06 12:00" },
    { "2012.10.07 10:00", "2012.10.07 12:00" },
  });

  TestRanges("Omitted time", {
    "Mo,We",
    "Mo&We", // error tolerance
    "Mo and We", // error tolerance
    "Mo-We; Tu off",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 0:00", "2012.10.02 0:00" },
    { "2012.10.03 0:00", "2012.10.04 0:00" },
  });

  TestRanges("Time ranges spanning midnight w/weekdays", {
    "We 22:00-02:00",
    "We 22:00-26:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.03 22:00", "2012.10.04 02:00" },
  });

  TestRanges("Exception rules", {
    "Mo-Fr 10:00-16:00; We 12:00-18:00"
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 16:00" },
    { "2012.10.02 10:00", "2012.10.02 16:00" },
    { "2012.10.03 12:00", "2012.10.03 18:00" }, // Not 10:00-18:00
    { "2012.10.04 10:00", "2012.10.04 16:00" },
    { "2012.10.05 10:00", "2012.10.05 16:00" },
  });
  // }}}

  // full range {{{
  TestRanges("Full range", {
    "00:00-24:00",
    "00:00-00:00",
    "12:00-12:00",
    "Mo-Su 00:00-24:00",
    "Tu-Mo 00:00-24:00",
    "We-Tu 00:00-24:00",
    "Th-We 00:00-24:00",
    "Fr-Th 00:00-24:00",
    "Sa-Fr 00:00-24:00",
    "Su-Sa 00:00-24:00",
    "24/7",
    "24/7; 24/7",     // Use value above.
    "0-24",           // Do not use. Returns warning.
    "midnight-24:00", // Do not use. Returns warning.
    "24 hours",       // Do not use. Returns warning.
    "open",
    "12:00-13:00; 24/7", // "12:00-13:00" is always ignored.
    "00:00-24:00,12:00-13:00", // "00:00-24:00" already matches entire day. "12:00-13:00" is pointless.
    "Mo-Fr,Sa,Su",
    // Is actually week stable, but check for that needs extra logic.
    "Jan-Dec",
    "Feb-Jan",
    "Dec-Nov",
    "week 1-53",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 0:00", "2012.10.08 0:00" },
  });

  TestRanges("24/7 as time interval alias (don’t use 24/7 as showen here)", {
    "Mo,We 00:00-24:00", // preferred because more explicit
    "Mo,We 24/7", // throws a warning
    "Mo,We open", // throws a warning
    "Mo,We", // throws a warning
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 0:00", "2012.10.02 0:00" },
    { "2012.10.03 0:00", "2012.10.04 0:00" },
  });
  // }}}

  // constrained weekdays {{{
  TestRanges("Constrained weekdays", {
    "We[4,5] 10:00-12:00",
    "We[4-5] 10:00-12:00",
    "We[4],We[5] 10:00-12:00",
    "We[4] 10:00-12:00; We[-1] 10:00-12:00",
    "We[-1,-2] 10:00-12:00",
  }, "2012.10.01 0:00", "2012.11.01 0:00", {
    { "2012.10.24 10:00", "2012.10.24 12:00" },
    { "2012.10.31 10:00", "2012.10.31 12:00" },
  });

  TestRanges("Calculations based on constrained weekdays", {
    // FIXME
    "Sa[-1] +3 days 10:00-12:00",
    "Sa[-1] +3 day 10:00-12:00", // 3 day is bad English but our library does tread them as synonym, but oh.prettifyValue fixes this of course ;)
  }, "2013.08.21 0:00", "2014.02.01 0:00", {
    { "2013.09.03 10:00", "2013.09.03 12:00" },
    { "2013.10.01 10:00", "2013.10.01 12:00" },
    { "2013.10.29 10:00", "2013.10.29 12:00" },
    { "2013.12.03 10:00", "2013.12.03 12:00" },
    { "2013.12.31 10:00", "2013.12.31 12:00" },
    { "2014.01.28 10:00", "2014.01.28 12:00" },
  });

  TestRanges("Calculations based on constrained weekdays: last weekend in month", {
    "Sa[-1],Sa[-1] +1 day 10:00-12:00",
  }, "2013.08.21 0:00", "2013.10.03 0:00", {
    { "2013.08.31 10:00", "2013.08.31 12:00" },
    { "2013.09.01 10:00", "2013.09.01 12:00" },
    { "2013.09.28 10:00", "2013.09.28 12:00" },
    { "2013.09.29 10:00", "2013.09.29 12:00" },
  });

  TestRanges("Calculations based on constrained weekdays: last weekend in month", {
    "Sa[-1],Sa[-1] +1 day",
  }, "2013.08.21 0:00", "2013.10.03 0:00", {
    { "2013.08.31 00:00", "2013.09.02 00:00" },
    { "2013.09.28 00:00", "2013.09.30 00:00" },
  });

  TestRanges("Calculations based on constrained weekdays", {
    "Sa[2] +3 days 10:00-12:00",
  }, "2013.08.21 0:00", "2013.12.01 0:00", {
    { "2013.09.17 10:00", "2013.09.17 12:00" },
    { "2013.10.15 10:00", "2013.10.15 12:00" },
    { "2013.11.12 10:00", "2013.11.12 12:00" },
  });

  TestRanges("Calculations based on constrained weekdays", {
    "Sa[1] -5 days",
  }, "2013.08.21 0:00", "2014.02.01 0:00", {
    { "2013.09.02 00:00", "2013.09.03 00:00" },
    { "2013.09.30 00:00", "2013.10.01 00:00" },
    { "2013.10.28 00:00", "2013.10.29 00:00" },
    { "2013.12.02 00:00", "2013.12.03 00:00" },
    { "2013.12.30 00:00", "2013.12.31 00:00" },
    { "2014.01.27 00:00", "2014.01.28 00:00" },
  });

  TestRanges("Calculations based on constrained weekdays", {
    "Su[-1] -1 day",
  }, "2013.08.21 0:00", "2014.02.01 0:00", {
    { "2013.08.24 00:00", "2013.08.25 00:00" },
    { "2013.09.28 00:00", "2013.09.29 00:00" },
    { "2013.10.26 00:00", "2013.10.27 00:00" },
    { "2013.11.23 00:00", "2013.11.24 00:00" },
    { "2013.12.28 00:00", "2013.12.29 00:00" },
    { "2014.01.25 00:00", "2014.01.26 00:00" },
  });

  TestRanges("Calculations based on constrained weekdays", {
    "Aug Su[-1] +1 day", // 25: Su;  26 Su +1 day
  }, "2013.08.01 0:00", "2013.10.08 0:00", {
    { "2013.08.26 00:00", "2013.08.27 00:00" },
  });

  TestRanges("Calculations based on constrained weekdays", {
    "Aug Su[-1] +1 day",
  }, "2013.08.26 8:00", "2013.10.08 0:00", {
    { "2013.08.26 08:00", "2013.08.27 00:00" },
  });

  TestRanges("Constrained weekday (complex real world example)", {
    "Apr-Oct: Su[2] 14:00-18:00; Aug Su[-1] -1 day 10:00-18:00, Aug: Su[-1] 10:00-18:00",
    "Apr-Oct: Su[2] 14:00-18:00; Aug Su[-1] -1 day 10:00-18:00; Aug: Su[-1] 10:00-18:00", // better use this instead
  }, "2013.08.01 0:00", "2013.10.08 0:00", {
    { "2013.08.11 14:00", "2013.08.11 18:00" },
    { "2013.08.24 10:00", "2013.08.24 18:00" },
    { "2013.08.25 10:00", "2013.08.25 18:00" },
    { "2013.09.08 14:00", "2013.09.08 18:00" },
  });
  // }}}

  // additional rules {{{
  TestRanges("Additional rules", {
    "Mo-Fr 10:00-16:00, We 12:00-18:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 16:00" },
    { "2012.10.02 10:00", "2012.10.02 16:00" },
    { "2012.10.03 10:00", "2012.10.03 18:00" },
    { "2012.10.04 10:00", "2012.10.04 16:00" },
    { "2012.10.05 10:00", "2012.10.05 16:00" },
  });

  TestRanges("Additional rules", {
    "Mo-Fr 08:00-12:00, We 14:00-18:00",
    "Mo-Fr 08:00-12:00, We 14:00-18:00; Su off",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 08:00", "2012.10.01 12:00" },
    { "2012.10.02 08:00", "2012.10.02 12:00" },
    { "2012.10.03 08:00", "2012.10.03 12:00" },
    { "2012.10.03 14:00", "2012.10.03 18:00" },
    { "2012.10.04 08:00", "2012.10.04 12:00" },
    { "2012.10.05 08:00", "2012.10.05 12:00" },
  });
  // }}}

  // fallback rules {{{
  TestRanges("Fallback group rules (unknown)", {
    "We-Fr 10:00-24:00 open \"it is open\" || \"please call\"",
    "We-Fr 10:00-24:00 open \"it is open\" || \"please call\" || closed \"should never appear\"",
    "We-Fr 10:00-24:00 open \"it is open\" || \"please call\" || unknown \"should never appear\"",
    "We-Fr 10:00-24:00 open \"it is open\" || \"please call\" || open \"should never appear\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.03 10:00", "please call", "true" },
    { "2012.10.03 10:00", "2012.10.04 00:00", "it is open" },
    { "2012.10.04 00:00", "2012.10.04 10:00", "please call", "true" },
    { "2012.10.04 10:00", "2012.10.05 00:00", "it is open" },
    { "2012.10.05 00:00", "2012.10.05 10:00", "please call", "true" },
    { "2012.10.05 10:00", "2012.10.06 00:00", "it is open" },
    { "2012.10.06 00:00", "2012.10.08 00:00", "please call", "true" },
  });

  TestRanges("Fallback group rules (unknown). Example for the tokenizer documentation.", {
    "We-Fr 10:00-24:00 open \"it is open\", Mo closed \"It‘s monday.\" || 2012 \"please call\"; Jan 1 open \"should never appear\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.03 10:00", "please call", "true" },
    { "2012.10.03 10:00", "2012.10.04 00:00", "it is open" },
    { "2012.10.04 00:00", "2012.10.04 10:00", "please call", "true" },
    { "2012.10.04 10:00", "2012.10.05 00:00", "it is open" },
    { "2012.10.05 00:00", "2012.10.05 10:00", "please call", "true" },
    { "2012.10.05 10:00", "2012.10.06 00:00", "it is open" },
    { "2012.10.06 00:00", "2012.10.08 00:00", "please call", "true" },
  });

  TestRanges("Fallback group rules", {
    "We-Fr 10:00-24:00 open \"first\" || We \"please call\" || open \"we are open!!!\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.03 00:00", "we are open!!!" }, // Mo,Tu
    { "2012.10.03 00:00", "2012.10.03 10:00", "please call", "true" },    // We
    { "2012.10.03 10:00", "2012.10.04 00:00", "first" },          // We
    { "2012.10.04 00:00", "2012.10.04 10:00", "we are open!!!" }, // Th
    { "2012.10.04 10:00", "2012.10.05 00:00", "first" },          // Th
    { "2012.10.05 00:00", "2012.10.05 10:00", "we are open!!!" }, // Fr
    { "2012.10.05 10:00", "2012.10.06 00:00", "first" },          // Fr
    { "2012.10.06 00:00", "2012.10.08 00:00", "we are open!!!" }, // Sa,Su
  });

  // example from Netzwolf
  TestRanges("Fallback group rules", {
    "Mo-Fr 08:00-12:00,14:00-18:00, Sa 09:00-13:00, PH off || Tu 06:00-06:00 open \"Notdienst\"", // Original value.
    "Mo-Fr 08:00-12:00,14:00-18:00; Sa 09:00-13:00; PH off || Tu 06:00-06:00 open \"Notdienst\"", // Use this instead.
    // Additional rule is not needed.
  }, "2013.10.01 0:00", "2013.10.08 0:00", {
    { "2013.10.01 06:00", "2013.10.01 08:00", "Notdienst" }, // Tu
    { "2013.10.01 08:00", "2013.10.01 12:00" },
    { "2013.10.01 12:00", "2013.10.01 14:00", "Notdienst" },
    { "2013.10.01 14:00", "2013.10.01 18:00" },
    { "2013.10.01 18:00", "2013.10.02 06:00", "Notdienst" },
    { "2013.10.02 08:00", "2013.10.02 12:00" }, // We
    { "2013.10.02 14:00", "2013.10.02 18:00" },
    { "2013.10.04 08:00", "2013.10.04 12:00" },
    { "2013.10.04 14:00", "2013.10.04 18:00" },
    { "2013.10.05 09:00", "2013.10.05 13:00" }, // Sa
    { "2013.10.07 08:00", "2013.10.07 12:00" }, // Mo
    { "2013.10.07 14:00", "2013.10.07 18:00" },
  });

  // example from Netzwolf
  TestRanges("Fallback group rules", {
    "Mo-Fr 08:00-11:00 || Th-Sa 12:00-13:00 open \"Emergency only\"",
    "Mo-Fr 08:00-11:00, Th-Sa 12:00-13:00 open \"Emergency only\"",
    // Additional rule does the same in this case because the second rule (including the time range) does not overlap the first rule.
    // Both variants are valid.
  }, "2013.10.01 0:00", "2013.10.08 0:00", {
    { "2013.10.01 08:00", "2013.10.01 11:00" },
    { "2013.10.02 08:00", "2013.10.02 11:00" },
    { "2013.10.03 08:00", "2013.10.03 11:00" },
    { "2013.10.03 12:00", "2013.10.03 13:00", "Emergency only" },
    { "2013.10.04 08:00", "2013.10.04 11:00" },
    { "2013.10.04 12:00", "2013.10.04 13:00", "Emergency only" },
    { "2013.10.05 12:00", "2013.10.05 13:00", "Emergency only" },
    { "2013.10.07 08:00", "2013.10.07 11:00" },
  });

  TestRanges("Fallback group rules, with some closed times", {
    "Mo,Tu,Th 09:00-12:00; Fr 14:00-17:30 || \"Termine nach Vereinbarung\"; We off",
    "Mo-Th 09:00-12:00; " "Fr 14:00-17:30 || \"Termine nach Vereinbarung\"; We off",
    "Mo-Th 09:00-12:00; " "Fr 14:00-17:30 || unknown \"Termine nach Vereinbarung\"; We off",
  }, "2013.10.01 0:00", "2013.10.08 0:00", {
    { "2013.10.01 00:00", "2013.10.01 09:00", "Termine nach Vereinbarung", "true" }, // 9
    { "2013.10.01 09:00", "2013.10.01 12:00" }, // Tu
    { "2013.10.01 12:00", "2013.10.02 00:00", "Termine nach Vereinbarung", "true" }, // 12
    // We off
    { "2013.10.03 00:00", "2013.10.03 09:00", "Termine nach Vereinbarung", "true" }, // 9
    { "2013.10.03 09:00", "2013.10.03 12:00" },
    { "2013.10.03 12:00", "2013.10.04 14:00", "Termine nach Vereinbarung", "true" }, // 12 + 14
    { "2013.10.04 14:00", "2013.10.04 17:30" }, // Fr
    { "2013.10.04 17:30", "2013.10.07 09:00", "Termine nach Vereinbarung" }, // 2.5 + 4 + 24 * 2 + 9
    { "2013.10.07 09:00", "2013.10.07 12:00" }, // Mo
    { "2013.10.07 12:00", "2013.10.08 00:00", "Termine nach Vereinbarung", "true" }, // 12
  });
  // }}}

  // week ranges {{{
  TestRanges("Week ranges", {
    "week 1,3 00:00-24:00",
    "week 1,3 00:00-24:00 || closed \"should not change the test result\"",
    // because comments for closed states are not compared (not returned by the high-level API).
    "week 1,3: 00:00-24:00",
    "week 1,week 3: 00:00-24:00",
    "week 1: 00:00-24:00; week 3: 00:00-24:00",
    "week 1; week 3",
    "week 1-3/2 00:00-24:00",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.02 00:00", "2012.01.09 00:00" },
    { "2012.01.16 00:00", "2012.01.23 00:00" },
    { "2012.12.31 00:00", "2013.01.01 00:00" },
  });

  TestRanges("Week ranges", {
    "week 2,4 00:00-24:00",
    "week 2-4/2 00:00-24:00",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.09 00:00", "2012.01.16 00:00" },
    { "2012.01.23 00:00", "2012.01.30 00:00" },
  });

  TestRanges("Week range limit", {
    "week 2-53",
    "week 2-53 00:00-24:00",
  }, "2012.01.01 0:00", "2014.01.01 0:00", {
    { "2012.01.01 00:00", "2012.01.02 00:00" }, // Checked against http://www.schulferien.org/kalenderwoche/kalenderwochen_2012.html
    { "2012.01.09 00:00", "2012.12.31 00:00" },
    { "2013.01.07 00:00", "2013.12.30 00:00" },
  });

  TestRanges("Week range full range", {
    "week 1-53",
    "week 1-53 00:00-24:00",
  }, "2012.01.01 0:00", "2014.01.01 0:00", {
    { "2012.01.01 00:00", "2014.01.01 00:00" },
  });

  TestRanges("Week range second week", {
    "week 2 00:00-24:00",
  }, "2012.01.01 0:00", "2014.01.01 0:00", {
    { "2012.01.09 00:00", "2012.01.16 00:00" },
    { "2013.01.07 00:00", "2013.01.14 00:00" },
  });

  TestRanges("Week range", {
    "week 2-53/2 We; week 1-53/2 Sa 00:00-24:00",
  }, "2012.01.01 0:00", "2014.01.01 0:00", {
    /* Long test on per day base {{{ */
    { "2012.01.07 00:00", "2012.01.08 00:00" }, // Sa, KW1
    { "2012.01.11 00:00", "2012.01.12 00:00" }, // We, KW2
    { "2012.01.21 00:00", "2012.01.22 00:00" }, // Sa, KW3
    { "2012.01.25 00:00", "2012.01.26 00:00" },
    { "2012.02.04 00:00", "2012.02.05 00:00" },
    { "2012.02.08 00:00", "2012.02.09 00:00" },
    { "2012.02.18 00:00", "2012.02.19 00:00" },
    { "2012.02.22 00:00", "2012.02.23 00:00" },
    { "2012.03.03 00:00", "2012.03.04 00:00" },
    { "2012.03.07 00:00", "2012.03.08 00:00" },
    { "2012.03.17 00:00", "2012.03.18 00:00" },
    { "2012.03.21 00:00", "2012.03.22 00:00" },
    { "2012.03.31 00:00", "2012.04.01 00:00" },
    { "2012.04.04 00:00", "2012.04.05 00:00" },
    { "2012.04.14 00:00", "2012.04.15 00:00" },
    { "2012.04.18 00:00", "2012.04.19 00:00" },
    { "2012.04.28 00:00", "2012.04.29 00:00" },
    { "2012.05.02 00:00", "2012.05.03 00:00" },
    { "2012.05.12 00:00", "2012.05.13 00:00" },
    { "2012.05.16 00:00", "2012.05.17 00:00" },
    { "2012.05.26 00:00", "2012.05.27 00:00" },
    { "2012.05.30 00:00", "2012.05.31 00:00" },
    { "2012.06.09 00:00", "2012.06.10 00:00" },
    { "2012.06.13 00:00", "2012.06.14 00:00" },
    { "2012.06.23 00:00", "2012.06.24 00:00" },
    { "2012.06.27 00:00", "2012.06.28 00:00" },
    { "2012.07.07 00:00", "2012.07.08 00:00" },
    { "2012.07.11 00:00", "2012.07.12 00:00" },
    { "2012.07.21 00:00", "2012.07.22 00:00" },
    { "2012.07.25 00:00", "2012.07.26 00:00" },
    { "2012.08.04 00:00", "2012.08.05 00:00" },
    { "2012.08.08 00:00", "2012.08.09 00:00" },
    { "2012.08.18 00:00", "2012.08.19 00:00" },
    { "2012.08.22 00:00", "2012.08.23 00:00" },
    { "2012.09.01 00:00", "2012.09.02 00:00" },
    { "2012.09.05 00:00", "2012.09.06 00:00" },
    { "2012.09.15 00:00", "2012.09.16 00:00" },
    { "2012.09.19 00:00", "2012.09.20 00:00" },
    { "2012.09.29 00:00", "2012.09.30 00:00" },
    { "2012.10.03 00:00", "2012.10.04 00:00" },
    { "2012.10.13 00:00", "2012.10.14 00:00" },
    { "2012.10.17 00:00", "2012.10.18 00:00" },
    { "2012.10.27 00:00", "2012.10.28 00:00" },
    { "2012.10.31 00:00", "2012.11.01 00:00" },
    { "2012.11.10 00:00", "2012.11.11 00:00" },
    { "2012.11.14 00:00", "2012.11.15 00:00" },
    { "2012.11.24 00:00", "2012.11.25 00:00" },
    { "2012.11.28 00:00", "2012.11.29 00:00" },
    { "2012.12.08 00:00", "2012.12.09 00:00" },
    { "2012.12.12 00:00", "2012.12.13 00:00" },
    { "2012.12.22 00:00", "2012.12.23 00:00" }, // Sa, KW51
    { "2012.12.26 00:00", "2012.12.27 00:00" }, // We, KW52
    { "2013.01.05 00:00", "2013.01.06 00:00" }, // Sa, KW01
    { "2013.01.09 00:00", "2013.01.10 00:00" },
    { "2013.01.19 00:00", "2013.01.20 00:00" },
    { "2013.01.23 00:00", "2013.01.24 00:00" },
    { "2013.02.02 00:00", "2013.02.03 00:00" },
    { "2013.02.06 00:00", "2013.02.07 00:00" },
    { "2013.02.16 00:00", "2013.02.17 00:00" },
    { "2013.02.20 00:00", "2013.02.21 00:00" },
    { "2013.03.02 00:00", "2013.03.03 00:00" },
    { "2013.03.06 00:00", "2013.03.07 00:00" },
    { "2013.03.16 00:00", "2013.03.17 00:00" },
    { "2013.03.20 00:00", "2013.03.21 00:00" },
    { "2013.03.30 00:00", "2013.03.31 00:00" },
    { "2013.04.03 00:00", "2013.04.04 00:00" },
    { "2013.04.13 00:00", "2013.04.14 00:00" },
    { "2013.04.17 00:00", "2013.04.18 00:00" },
    { "2013.04.27 00:00", "2013.04.28 00:00" },
    { "2013.05.01 00:00", "2013.05.02 00:00" },
    { "2013.05.11 00:00", "2013.05.12 00:00" },
    { "2013.05.15 00:00", "2013.05.16 00:00" },
    { "2013.05.25 00:00", "2013.05.26 00:00" },
    { "2013.05.29 00:00", "2013.05.30 00:00" },
    { "2013.06.08 00:00", "2013.06.09 00:00" },
    { "2013.06.12 00:00", "2013.06.13 00:00" },
    { "2013.06.22 00:00", "2013.06.23 00:00" },
    { "2013.06.26 00:00", "2013.06.27 00:00" },
    { "2013.07.06 00:00", "2013.07.07 00:00" },
    { "2013.07.10 00:00", "2013.07.11 00:00" },
    { "2013.07.20 00:00", "2013.07.21 00:00" },
    { "2013.07.24 00:00", "2013.07.25 00:00" },
    { "2013.08.03 00:00", "2013.08.04 00:00" },
    { "2013.08.07 00:00", "2013.08.08 00:00" },
    { "2013.08.17 00:00", "2013.08.18 00:00" },
    { "2013.08.21 00:00", "2013.08.22 00:00" },
    { "2013.08.31 00:00", "2013.09.01 00:00" },
    { "2013.09.04 00:00", "2013.09.05 00:00" },
    { "2013.09.14 00:00", "2013.09.15 00:00" },
    { "2013.09.18 00:00", "2013.09.19 00:00" },
    { "2013.09.28 00:00", "2013.09.29 00:00" },
    { "2013.10.02 00:00", "2013.10.03 00:00" },
    { "2013.10.12 00:00", "2013.10.13 00:00" },
    { "2013.10.16 00:00", "2013.10.17 00:00" },
    { "2013.10.26 00:00", "2013.10.27 00:00" },
    { "2013.10.30 00:00", "2013.10.31 00:00" },
    { "2013.11.09 00:00", "2013.11.10 00:00" },
    { "2013.11.13 00:00", "2013.11.14 00:00" },
    { "2013.11.23 00:00", "2013.11.24 00:00" },
    { "2013.11.27 00:00", "2013.11.28 00:00" },
    { "2013.12.07 00:00", "2013.12.08 00:00" },
    { "2013.12.11 00:00", "2013.12.12 00:00" },
    { "2013.12.21 00:00", "2013.12.22 00:00" }, // Sa, KW51
    { "2013.12.25 00:00", "2013.12.26 00:00" }, // We, KW52
    /* }}} */
  });

  std::initializer_list<std::vector<std::string>> week_range_result = {
    { "2012.01.23 00:00", "2012.04.23 00:00" },
    { "2013.01.21 00:00", "2013.04.22 00:00" },
    { "2014.01.20 00:00", "2014.04.21 00:00" },
    { "2015.01.19 00:00", "2015.04.20 00:00" },
    { "2016.01.25 00:00", "2016.04.25 00:00" },
    { "2017.01.23 00:00", "2017.04.24 00:00" },
    // Checked against http://www.schulferien.org/kalenderwoche/kalenderwochen_2017.html
  };

  TestRanges("Week range (beginning in last year)", {
    "week 4-16",
  }, "2011.12.30 0:00", "2018.01.01 0:00", week_range_result);

  TestRanges("Week range (beginning in matching year)", {
    "week 4-16",
  }, "2012.01.01 0:00", "2018.01.01 0:00", week_range_result);

  TestRanges("Week range first week", {
    "week 1",
  }, "2014.12.01 0:00", "2015.02.01 0:00", {
    { "2014.12.29 00:00", "2015.01.05 00:00" },
  });

  TestRanges("Week range first week", {
    "week 1",
    "week 1 open",
    "week 1 00:00-24:00",
  }, "2012.12.01 0:00", "2024.02.01 0:00", {
    { "2012.12.31 00:00", "2013.01.07 00:00" },
    { "2013.12.30 00:00", "2014.01.06 00:00" },
    { "2014.12.29 00:00", "2015.01.05 00:00" },
    { "2016.01.04 00:00", "2016.01.11 00:00" },
    { "2017.01.02 00:00", "2017.01.09 00:00" },
    { "2018.01.01 00:00", "2018.01.08 00:00" },
    { "2018.12.31 00:00", "2019.01.07 00:00" },
    { "2019.12.30 00:00", "2020.01.06 00:00" },
    { "2021.01.04 00:00", "2021.01.11 00:00" },
    { "2022.01.03 00:00", "2022.01.10 00:00" },
    { "2023.01.02 00:00", "2023.01.09 00:00" },
    { "2024.01.01 00:00", "2024.01.08 00:00" },
    // Checked against http://www.schulferien.org/kalenderwoche/kalenderwochen_2024.html
  });

  TestRanges("Week range first week", {
    "week 1 00:00-23:59",
  }, "2012.12.01 0:00", "2024.02.01 0:00", {
    /* Long test on per day base {{{ */
    { "2012.12.31 00:00", "2012.12.31 23:59" },
    { "2013.01.01 00:00", "2013.01.01 23:59" },
    { "2013.01.02 00:00", "2013.01.02 23:59" },
    { "2013.01.03 00:00", "2013.01.03 23:59" },
    { "2013.01.04 00:00", "2013.01.04 23:59" },
    { "2013.01.05 00:00", "2013.01.05 23:59" },
    { "2013.01.06 00:00", "2013.01.06 23:59" },
    { "2013.12.30 00:00", "2013.12.30 23:59" },
    { "2013.12.31 00:00", "2013.12.31 23:59" },
    { "2014.01.01 00:00", "2014.01.01 23:59" },
    { "2014.01.02 00:00", "2014.01.02 23:59" },
    { "2014.01.03 00:00", "2014.01.03 23:59" },
    { "2014.01.04 00:00", "2014.01.04 23:59" },
    { "2014.01.05 00:00", "2014.01.05 23:59" },
    { "2014.12.29 00:00", "2014.12.29 23:59" },
    { "2014.12.30 00:00", "2014.12.30 23:59" },
    { "2014.12.31 00:00", "2014.12.31 23:59" },
    { "2015.01.01 00:00", "2015.01.01 23:59" },
    { "2015.01.02 00:00", "2015.01.02 23:59" },
    { "2015.01.03 00:00", "2015.01.03 23:59" },
    { "2015.01.04 00:00", "2015.01.04 23:59" },
    { "2016.01.04 00:00", "2016.01.04 23:59" },
    { "2016.01.05 00:00", "2016.01.05 23:59" },
    { "2016.01.06 00:00", "2016.01.06 23:59" },
    { "2016.01.07 00:00", "2016.01.07 23:59" },
    { "2016.01.08 00:00", "2016.01.08 23:59" },
    { "2016.01.09 00:00", "2016.01.09 23:59" },
    { "2016.01.10 00:00", "2016.01.10 23:59" },
    { "2017.01.02 00:00", "2017.01.02 23:59" },
    { "2017.01.03 00:00", "2017.01.03 23:59" },
    { "2017.01.04 00:00", "2017.01.04 23:59" },
    { "2017.01.05 00:00", "2017.01.05 23:59" },
    { "2017.01.06 00:00", "2017.01.06 23:59" },
    { "2017.01.07 00:00", "2017.01.07 23:59" },
    { "2017.01.08 00:00", "2017.01.08 23:59" },
    { "2018.01.01 00:00", "2018.01.01 23:59" },
    { "2018.01.02 00:00", "2018.01.02 23:59" },
    { "2018.01.03 00:00", "2018.01.03 23:59" },
    { "2018.01.04 00:00", "2018.01.04 23:59" },
    { "2018.01.05 00:00", "2018.01.05 23:59" },
    { "2018.01.06 00:00", "2018.01.06 23:59" },
    { "2018.01.07 00:00", "2018.01.07 23:59" },
    { "2018.12.31 00:00", "2018.12.31 23:59" },
    { "2019.01.01 00:00", "2019.01.01 23:59" },
    { "2019.01.02 00:00", "2019.01.02 23:59" },
    { "2019.01.03 00:00", "2019.01.03 23:59" },
    { "2019.01.04 00:00", "2019.01.04 23:59" },
    { "2019.01.05 00:00", "2019.01.05 23:59" },
    { "2019.01.06 00:00", "2019.01.06 23:59" },
    { "2019.12.30 00:00", "2019.12.30 23:59" },
    { "2019.12.31 00:00", "2019.12.31 23:59" },
    { "2020.01.01 00:00", "2020.01.01 23:59" },
    { "2020.01.02 00:00", "2020.01.02 23:59" },
    { "2020.01.03 00:00", "2020.01.03 23:59" },
    { "2020.01.04 00:00", "2020.01.04 23:59" },
    { "2020.01.05 00:00", "2020.01.05 23:59" },
    { "2021.01.04 00:00", "2021.01.04 23:59" },
    { "2021.01.05 00:00", "2021.01.05 23:59" },
    { "2021.01.06 00:00", "2021.01.06 23:59" },
    { "2021.01.07 00:00", "2021.01.07 23:59" },
    { "2021.01.08 00:00", "2021.01.08 23:59" },
    { "2021.01.09 00:00", "2021.01.09 23:59" },
    { "2021.01.10 00:00", "2021.01.10 23:59" },
    { "2022.01.03 00:00", "2022.01.03 23:59" },
    { "2022.01.04 00:00", "2022.01.04 23:59" },
    { "2022.01.05 00:00", "2022.01.05 23:59" },
    { "2022.01.06 00:00", "2022.01.06 23:59" },
    { "2022.01.07 00:00", "2022.01.07 23:59" },
    { "2022.01.08 00:00", "2022.01.08 23:59" },
    { "2022.01.09 00:00", "2022.01.09 23:59" },
    { "2023.01.02 00:00", "2023.01.02 23:59" },
    { "2023.01.03 00:00", "2023.01.03 23:59" },
    { "2023.01.04 00:00", "2023.01.04 23:59" },
    { "2023.01.05 00:00", "2023.01.05 23:59" },
    { "2023.01.06 00:00", "2023.01.06 23:59" },
    { "2023.01.07 00:00", "2023.01.07 23:59" },
    { "2023.01.08 00:00", "2023.01.08 23:59" },
    { "2024.01.01 00:00", "2024.01.01 23:59" },
    { "2024.01.02 00:00", "2024.01.02 23:59" },
    { "2024.01.03 00:00", "2024.01.03 23:59" },
    { "2024.01.04 00:00", "2024.01.04 23:59" },
    { "2024.01.05 00:00", "2024.01.05 23:59" },
    { "2024.01.06 00:00", "2024.01.06 23:59" },
    { "2024.01.07 00:00", "2024.01.07 23:59" },
    /* }}} */
  });
  // }}}

  // full months/month ranges {{{
  TestRanges("Only in one month of the year", {
    "Apr 08:00-12:00",
    "Apr: 08:00-12:00",
  }, "2013.04.28 0:00", "2014.04.03 0:00", {
    { "2013.04.28 08:00", "2013.04.28 12:00" },
    { "2013.04.29 08:00", "2013.04.29 12:00" },
    { "2013.04.30 08:00", "2013.04.30 12:00" },
    { "2014.04.01 08:00", "2014.04.01 12:00" },
    { "2014.04.02 08:00", "2014.04.02 12:00" },
  });

  TestRanges("Month ranges", {
    "Nov-Feb 00:00-24:00",
    "Nov-Feb00:00-24:00",
    "Nov-Feb",
    "Nov-Feb 0-24", // Do not use. Returns warning and corrected value.
    "Nov-Feb: 00:00-24:00",
    "Jan,Feb,Nov,Dec 00:00-24:00",
    "00:00-24:00; Mar-Oct off",
    "open; Mar-Oct off",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.01 00:00", "2012.03.01 00:00" },
    { "2012.11.01 00:00", "2013.01.01 00:00" },
  });

  TestRanges("Month ranges", {
    "Nov-Nov 00:00-24:00",
    "Nov-Nov",
    "2012 Nov-Nov",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.11.01 00:00", "2012.12.01 00:00" },
  });
  // }}}

  // monthday ranges {{{
  TestRanges("Month ranges", {
    "Jan 1,Dec 24-25; Nov Th[4]",
    "Jan 1,Dec 24,25; Nov Th[4]", // Was supported by time_domain as well.
    "2012 Jan 1,2012 Dec 24-25; 2012 Nov Th[4]",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.01 00:00", "2012.01.02 00:00" },
    { "2012.11.22 00:00", "2012.11.23 00:00" },
    { "2012.12.24 00:00", "2012.12.26 00:00" },
  });

  TestRanges("Month ranges", {
    "Jan 1,Dec 11,Dec 15-17,Dec 19-23/2,Dec 24-25",
    "Jan 1,Dec 11,15-17,19-23/2,24,25", // Was supported by time_domain as well.
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.01 00:00", "2012.01.02 00:00" },
    { "2012.12.11 00:00", "2012.12.12 00:00" },
    { "2012.12.15 00:00", "2012.12.18 00:00" },
    { "2012.12.19 00:00", "2012.12.20 00:00" },
    { "2012.12.21 00:00", "2012.12.22 00:00" },
    { "2012.12.23 00:00", "2012.12.26 00:00" },
  });

  TestRanges("Monthday ranges", {
    "Jan 23-31 00:00-24:00; Feb 1-12 00:00-24:00",
    "Jan 23-Feb 12 00:00-24:00",
    "Jan 23-Feb 12: 00:00-24:00",
    "2012 Jan 23-2012 Feb 12 00:00-24:00",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.23 0:00", "2012.02.13 00:00" },
  });

  TestRanges("Monthday ranges", {
    "Jan 31-Feb 1,Aug 00:00-24:00", // FIXME: Also fails in 9f323b9d06720b6efffc7420023e746ff8f1b309.
    "Jan 31-Feb 1,Aug: 00:00-24:00",
    "Aug,Jan 31-Feb 1",
    "Jan 31-Feb 1; Aug",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.31 00:00", "2012.02.02 00:00" },
    { "2012.08.01 00:00", "2012.09.01 00:00" },
  });

  TestRanges("Monthday ranges", {
    "Dec 24,Jan 2: 18:00-22:00",
    "Dec 24,Jan 2: 18:00-22:00; Jan 20: off",
    "Dec 24,Jan 2 18:00-22:00",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.02 18:00", "2012.01.02 22:00" },
    { "2012.12.24 18:00", "2012.12.24 22:00" },
  });

  TestRanges("Monthday ranges (with year)", {
    "2012 Jan 23-31 00:00-24:00; 2012 Feb 1-12 00:00-24:00",
  }, "2012.01.01 0:00", "2015.01.01 0:00", {
    { "2012.01.23 0:00", "2012.02.13 00:00" },
  });

  TestRanges("Monthday ranges spanning year boundary", {
    "Dec 31-Jan 1",
  }, "2012.01.01 0:00", "2014.01.01 0:00", {
    { "2012.01.01 0:00", "2012.01.02 00:00" },
    { "2012.12.31 0:00", "2013.01.02 00:00" },
    { "2013.12.31 0:00", "2014.01.01 00:00" },
  });

  TestRanges("Full day (with year)", {
    "2013 Dec 31,2014 Jan 5",
    "2013 Dec 31; 2014 Jan 5",
    "2013/10 Dec 31; 2014/10 Jan 5", // force to use parseYearRange
  }, "2011.01.01 0:00", "2015.01.01 0:00", {
    { "2013.12.31 00:00", "2014.01.01 00:00" },
    { "2014.01.05 00:00", "2014.01.06 00:00" },
  });

  TestRanges("Date range which only applies for one year", {
    "2013 Dec 31",
    "2013 Dec 31; 2014 Jan 5; 2014+ off",
  }, "2011.01.01 0:00", "2015.01.01 0:00", {
    { "2013.12.31 0:00", "2014.01.01 00:00" },
  });

  TestRanges("Monthday (with year) ranges spanning year boundary", {
    "2013 Dec 31-2014 Jan 2",
    "open; 2010 Jan 1-2013 Dec 30 off; 2014 Jan 3-2016 Jan 1 off",
  }, "2011.01.01 0:00", "2015.01.01 0:00", {
    { "2013.12.31 0:00", "2014.01.03 00:00" },
  });

  TestRanges("Monthday ranges with constrained weekday", {
    "Jan Su[2]-Jan 15",
  }, "2012.01.01 0:00", "2015.01.01 0:00", {
    { "2012.01.08 00:00", "2012.01.16 00:00" }, // 8
    { "2013.01.13 00:00", "2013.01.16 00:00" }, // 3
    { "2014.01.12 00:00", "2014.01.16 00:00" }, // 4
  });

  TestRanges("Monthday ranges with constrained weekday", {
    "Jan 20-Jan Su[-1]",
  }, "2012.01.01 0:00", "2015.01.01 0:00", {
    { "2012.01.20 00:00", "2012.01.29 00:00" },
    { "2013.01.20 00:00", "2013.01.27 00:00" },
    { "2014.01.20 00:00", "2014.01.26 00:00" },
  });

  TestRanges("Monthday ranges with constrained weekday", {
    "Jan Su[1] +2 days-Jan Su[3] -2 days", // just for testing, can probably be expressed better
  }, "2012.01.01 0:00", "2015.01.01 0:00", {
    { "2012.01.03 00:00", "2012.01.13 00:00" },
    { "2013.01.08 00:00", "2013.01.18 00:00" },
    { "2014.01.07 00:00", "2014.01.17 00:00" },
  });

  TestRanges("Monthday ranges with constrained weekday spanning year", {
    "Dec 20-Dec Su[-1] +4 days",
  }, "2011.01.01 0:00", "2015.01.01 0:00", {
    { "2011.12.20 00:00", "2011.12.29 00:00" },
    { "2012.12.20 00:00", "2013.01.03 00:00" },
    { "2013.12.20 00:00", "2014.01.02 00:00" },
    { "2014.12.20 00:00", "2015.01.01 00:00" },
  });

  TestRanges("Monthday ranges with constrained", {
    "Nov Su[-1]-Dec Su[1] -1 day",
  }, "2011.01.01 0:00", "2015.01.01 0:00", {
    { "2011.11.27 00:00", "2011.12.03 00:00" },
    { "2012.11.25 00:00", "2012.12.01 00:00" },
    { "2013.11.24 00:00", "2013.11.30 00:00" },
    { "2014.11.30 00:00", "2014.12.06 00:00" },
  });

  TestRanges("Monthday ranges", {
    "Mar Su[-1]-Oct Su[-1] -1 day open; Oct Su[-1]-Mar Su[-1] -1 day off",
    "Mar Su[-1]-Oct Su[-1] -1 day open",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.03.25 00:00", "2012.10.27 00:00" },
  });

  TestRanges("Month ranges with year", {
    "2012 Jan 10-15,Jan 11",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.10 00:00", "2012.01.16 00:00" },
  });

  TestRanges("Complex monthday ranges", {
    "Jan 23-31,Feb 1-12 00:00-24:00",
    "Jan 23-Feb 11,Feb 12 00:00-24:00", // preferred
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.23 0:00", "2012.02.13 00:00" },
  });

  // leap years {{{
  TestRanges("Leap year monthday", {
    "2016 Feb 29",
  }, "2012.01.01 0:00", "2019.01.01 0:00", {
    { "2016.02.29 00:00", "2016.03.01 00:00" },
  });

  TestRanges("Leap year monthday", {
    "2015 Feb 29",
  }, "2012.01.01 0:00", "2019.01.01 0:00", {
  });

  TestRanges("Last day in month", {
    // something like this:
    "Jan 31,Mar 31,Apr 30,May 31,Jun 30,Jul 31,Aug 31,Sep 30,Oct 31,Nov 30,Dec 31 open \"last day in month\"; "
    // The year selector can also be used but is not required as the monthday selector should only match if day exists.
    "Feb 29 open \"last day in month (Feb, leap year)\"; 2009/4,2010/4,2011/4 Feb 28 open \"last day in month (Feb, not leap year)\"",
    // There is no shortcut yet. Make sure that you include comments to help other mappers to understand this and to find and replace the value easier if a sorter version was introduced.
    "Jan 31,Mar 31,Apr 30,May 31,Jun 30,Jul 31,Aug 31,Sep 30,Oct 31,Nov 30,Dec 31 open \"last day in month\"; 2008/4 Feb 29 open \"last day in month (Feb, leap year)\"; 2009/4,2010/4,2011/4 Feb 28 open \"last day in month (Feb, not leap year)\"",
  }, "2012.01.01 0:00", "2014.01.01 0:00", {
    { "2012.01.31 00:00", "2012.02.01 00:00", "last day in month" },
    { "2012.02.29 00:00", "2012.03.01 00:00", "last day in month (Feb, leap year)" },
    { "2012.03.31 00:00", "2012.04.01 00:00", "last day in month" },
    { "2012.04.30 00:00", "2012.05.01 00:00", "last day in month" },
    { "2012.05.31 00:00", "2012.06.01 00:00", "last day in month" },
    { "2012.06.30 00:00", "2012.07.01 00:00", "last day in month" },
    { "2012.07.31 00:00", "2012.08.01 00:00", "last day in month" },
    { "2012.08.31 00:00", "2012.09.01 00:00", "last day in month" },
    { "2012.09.30 00:00", "2012.10.01 00:00", "last day in month" },
    { "2012.10.31 00:00", "2012.11.01 00:00", "last day in month" },
    { "2012.11.30 00:00", "2012.12.01 00:00", "last day in month" },
    { "2012.12.31 00:00", "2013.01.01 00:00", "last day in month" },
    { "2013.01.31 00:00", "2013.02.01 00:00", "last day in month" },
    { "2013.02.28 00:00", "2013.03.01 00:00", "last day in month (Feb, not leap year)" },
    { "2013.03.31 00:00", "2013.04.01 00:00", "last day in month" },
    { "2013.04.30 00:00", "2013.05.01 00:00", "last day in month" },
    { "2013.05.31 00:00", "2013.06.01 00:00", "last day in month" },
    { "2013.06.30 00:00", "2013.07.01 00:00", "last day in month" },
    { "2013.07.31 00:00", "2013.08.01 00:00", "last day in month" },
    { "2013.08.31 00:00", "2013.09.01 00:00", "last day in month" },
    { "2013.09.30 00:00", "2013.10.01 00:00", "last day in month" },
    { "2013.10.31 00:00", "2013.11.01 00:00", "last day in month" },
    { "2013.11.30 00:00", "2013.12.01 00:00", "last day in month" },
    { "2013.12.31 00:00", "2014.01.01 00:00", "last day in month" },
  });
  // }}}

  // periodical monthdays {{{
  TestRanges("Periodical monthdays", {
    "Jan 1-31/8 00:00-24:00",
    "Jan 1-31/8: 00:00-24:00",
    "Jan 1-31/8",
    "2012 Jan 1-31/8",
    "2012 Jan 1-31/8; 2010 Dec 1-31/8",
    "2012 Jan 1-31/8; 2015 Dec 1-31/8",
    "2012 Jan 1-31/8; 2025 Dec 1-31/8",
    "2012 Jan 1-31/8: 00:00-24:00",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.01 0:00", "2012.01.02 00:00" },
    { "2012.01.09 0:00", "2012.01.10 00:00" },
    { "2012.01.17 0:00", "2012.01.18 00:00" },
    { "2012.01.25 0:00", "2012.01.26 00:00" },
  });

  TestRanges("Periodical monthdays", {
    "Jan 10-31/7",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.10 0:00", "2012.01.11 00:00" },
    { "2012.01.17 0:00", "2012.01.18 00:00" },
    { "2012.01.24 0:00", "2012.01.25 00:00" },
    { "2012.01.31 0:00", "2012.02.01 00:00" },
  });
  // }}}

  // }}}

  // year ranges {{{
  TestRanges("Date range which only applies for specific year", {
    // FIXME
    "2013,2015,2050-2053,2055/2,2020-2029/3,2060+ Jan 1", // Used on the demo page.
    "2013,2015,2050-2053,2055/2,2020-2029/3,2060+ Jan 1 Mo-Su",
  }, "2011.01.01 0:00", "2065.01.01 0:00", {
    { "2013.01.01 00:00", "2013.01.02 00:00" },
    { "2015.01.01 00:00", "2015.01.02 00:00" },
    { "2020.01.01 00:00", "2020.01.02 00:00" },
    { "2023.01.01 00:00", "2023.01.02 00:00" },
    { "2026.01.01 00:00", "2026.01.02 00:00" },
    { "2029.01.01 00:00", "2029.01.02 00:00" },
    { "2050.01.01 00:00", "2050.01.02 00:00" },
    { "2051.01.01 00:00", "2051.01.02 00:00" },
    { "2052.01.01 00:00", "2052.01.02 00:00" },
    { "2053.01.01 00:00", "2053.01.02 00:00" },
    { "2055.01.01 00:00", "2055.01.02 00:00" },
    { "2057.01.01 00:00", "2057.01.02 00:00" },
    { "2059.01.01 00:00", "2059.01.02 00:00" },
    { "2060.01.01 00:00", "2060.01.02 00:00" },
    { "2061.01.01 00:00", "2061.01.02 00:00" },
    { "2062.01.01 00:00", "2062.01.02 00:00" },
    { "2063.01.01 00:00", "2063.01.02 00:00" },
    { "2064.01.01 00:00", "2064.01.02 00:00" },
  });

  TestRanges("Date range which only applies for specific year", {
    "2060+",
  }, "2011.01.01 0:00", "2065.01.01 0:00", {
    { "2060.01.01 00:00", "2065.01.01 00:00" },
  });

  TestRanges("Date range which only applies for specific year", {
    "2040-2050",
  }, "2011.01.01 0:00", "2065.01.01 0:00", {
    { "2040.01.01 00:00", "2051.01.01 00:00" },
  });

  TestRanges("Date range which only applies for specific year", {
    "2012-2016",
  }, "2011.01.01 0:00", "2065.01.01 0:00", {
    { "2012.01.01 00:00", "2017.01.01 00:00" },
  });
  // }}}

  // selector combination and order {{{
  TestRanges("Selector combination", {
    "week 2 We",            // week + weekday
    "Jan 11-Jan 11 week 2", // week + monthday
    "Jan 11-Jan 11: week 2: 00:00-24:00", // week + monthday
    "Jan 11 week 2",        // week + monthday
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.11 0:00", "2012.01.12 00:00" },
  });

  TestRanges("Selector combination", {
    "Jan week 2",           // week + month
    "Jan-Feb Jan 9-Jan 15", // month + monthday
    "Jan-Feb Jan 9-15",     // month + monthday
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.09 0:00", "2012.01.16 00:00" },
  });

  TestRanges("Selector combination", {
    "Jan We",           // month + weekday
    "Jan 2-27 We",      // weekday + monthday
    "Dec 30-Jan 27 We", // weekday + monthday
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.01.04 0:00", "2012.01.05 00:00" },
    { "2012.01.11 0:00", "2012.01.12 00:00" },
    { "2012.01.18 0:00", "2012.01.19 00:00" },
    { "2012.01.25 0:00", "2012.01.26 00:00" },
  });

  TestRanges("Selector order", {
    // Result should not depend on selector order although there are some best practices:
    // Use the selector types which can cover the biggest range first e.g. year before month.
    "Feb week 5",
    "Feb week 5 00:00-24:00",
    "Feb week 5: 00:00-24:00",
    "Feb week 5 Mo-Su 00:00-24:00",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.02.01 0:00", "2012.02.06 00:00" },
  });

  TestRanges("Selector order", {
    "Feb week 6",
    "Feb week 6 open",
  }, "2012.01.01 0:00", "2013.01.01 0:00", {
    { "2012.02.06 0:00", "2012.02.13 00:00" },
  });
  // }}}

  // comments {{{

  TestRanges("Additional comment \"Nach Vereinbarung\"", {
    "Mo-Fr 08:00-12:00 open \"Kein Termin erforderlich\", Mo-Fr 13:00-17:00 open \"Nach Vereinbarung\"",
    "Mo-Fr 08:00-12:00 open \"Kein Termin erforderlich\", Mo-Fr 13:00-17:00 open nach_vereinbarung",
    "Mo-Fr 08:00-12:00 open \"Kein Termin erforderlich\", Mo-Fr 13:00-17:00 open nach Vereinbarung",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 08:00", "2012.10.01 12:00", "Kein Termin erforderlich" },
    { "2012.10.01 13:00", "2012.10.01 17:00", "Nach Vereinbarung" },
  });

  TestRanges("Additional comment \"on appointment\"", {
    "Mo-Fr 08:00-12:00 open \"appointment not needed\", Mo-Fr 13:00-17:00 open \"on appointment\"",
    "Mo-Fr 08:00-12:00 open \"appointment not needed\", Mo-Fr 13:00-17:00 open on_appointment",
    "Mo-Fr 08:00-12:00 open \"appointment not needed\", Mo-Fr 13:00-17:00 open on appointment",
    "Mo-Fr 08:00-12:00 open \"appointment not needed\", Mo-Fr 13:00-17:00 open by_appointment",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 08:00", "2012.10.01 12:00", "appointment not needed" },
    { "2012.10.01 13:00", "2012.10.01 17:00", "on appointment" },
  });

  TestRanges("Additional comments", {
    "Mo,Tu 10:00-16:00 open \"no warranty\"; We 12:00-18:00 open \"female only\"; Th closed \"Not open because we are coding :)\"; Fr 10:00-16:00 open \"male only\"; Sa 10:00-12:00 \"Maybe open. Call us.\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 16:00", "no warranty" },
    { "2012.10.02 10:00", "2012.10.02 16:00", "no warranty" },
    { "2012.10.03 12:00", "2012.10.03 18:00", "female only" },
    { "2012.10.05 10:00", "2012.10.05 16:00", "male only" },
    { "2012.10.06 10:00", "2012.10.06 12:00", "Maybe open. Call us.", "true" },
  });

  TestRanges("Additional comments for unknown", {
    "Sa 10:00-12:00 \"Maybe open. Call us. (testing special tokens in comment: ; ;; ' || | test end)\"",
    "Sa 10:00-12:00 unknown \"Maybe open. Call us. (testing special tokens in comment: ; ;; ' || | test end)\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.06 10:00", "2012.10.06 12:00", "Maybe open. Call us. (testing special tokens in comment: ; ;; ' || | test end)", "true" },
  });

  TestRanges("Date overwriting with additional comments for unknown ", {
    "Mo-Fr 10:00-20:00 unknown \"Maybe\"; We 10:00-16:00 \"Maybe open. Call us.\"",
    "Mo-Fr 10:00-20:00 unknown \"Maybe\"; We \"Maybe open. Call us.\" 10:00-16:00",
    "Mo-Fr 10:00-20:00 unknown \"Maybe\"; \"Maybe open. Call us.\" We 10:00-16:00",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 10:00", "2012.10.01 20:00", "Maybe", "true" },
    { "2012.10.02 10:00", "2012.10.02 20:00", "Maybe", "true" },
    { "2012.10.03 10:00", "2012.10.03 16:00", "Maybe open. Call us.", "true" },
    { "2012.10.04 10:00", "2012.10.04 20:00", "Maybe", "true" },
    { "2012.10.05 10:00", "2012.10.05 20:00", "Maybe", "true" },
  });

  TestRanges("Additional comments with time ranges spanning midnight", {
    "22:00-26:00; We 12:00-14:00 unknown \"Maybe open. Call us.\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 02:00" },
    { "2012.10.01 22:00", "2012.10.02 02:00" },
    { "2012.10.02 22:00", "2012.10.03 00:00" },
    { "2012.10.03 12:00", "2012.10.03 14:00", "Maybe open. Call us.", "true" },
    { "2012.10.04 00:00", "2012.10.04 02:00" },
    { "2012.10.04 22:00", "2012.10.05 02:00" },
    { "2012.10.05 22:00", "2012.10.06 02:00" },
    { "2012.10.06 22:00", "2012.10.07 02:00" },
    { "2012.10.07 22:00", "2012.10.08 00:00" },
  });

  TestRanges("Additional comments for closed with time ranges spanning midnight", {
    "22:00-26:00; We 12:00-14:00 off \"Not open because we are too tired\"",
    "22:00-26:00; We 12:00-14:00 closed \"Not open because we are too tired\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 02:00" },
    { "2012.10.01 22:00", "2012.10.02 02:00" },
    { "2012.10.02 22:00", "2012.10.03 02:00" },
    { "2012.10.03 22:00", "2012.10.04 02:00" },
    { "2012.10.04 22:00", "2012.10.05 02:00" },
    { "2012.10.05 22:00", "2012.10.06 02:00" },
    { "2012.10.06 22:00", "2012.10.07 02:00" },
    { "2012.10.07 22:00", "2012.10.08 00:00" },
  });

  TestRanges("Additional comments combined with additional rules", {
    "Mo 12:00-14:00 open \"female only\", Mo 14:00-16:00 open \"male only\"",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 12:00", "2012.10.01 14:00", "female only" },
    { "2012.10.01 14:00", "2012.10.01 16:00", "male only" },
  });

  // did only not work in browser: drawTable
  TestRanges("Additional comments combined with months", {
    "Apr-Sep; Oct-Dec \"on request\"",
    "Apr-Sep; Oct-Dec\"on request\"",
    "Oct-Dec \"on request\"; Apr-Sep",
  }, "2012.07.01 0:00", "2012.11.01 0:00", {
    { "2012.07.01 00:00", "2012.10.01 00:00" },
    { "2012.10.01 00:00", "2012.11.01 00:00", "on request", "true" },
  });
  // }}}

  TestRanges("Warnings corrected to additional rule (real world example)", {
    "Mo-Fr 09:00-12:00, Mo,Tu,Th 15:00-18:00", // reference value for prettify
    "Mo – Fr: 9 – 12 Uhr und Mo, Di, Do: 15 – 18 Uhr",
  }, "2014.09.01 0:00", "2014.09.08 0:00", {
    { "2014.09.01 09:00", "2014.09.01 12:00" },
    { "2014.09.01 15:00", "2014.09.01 18:00" },
    { "2014.09.02 09:00", "2014.09.02 12:00" },
    { "2014.09.02 15:00", "2014.09.02 18:00" },
    { "2014.09.03 09:00", "2014.09.03 12:00" },
    { "2014.09.04 09:00", "2014.09.04 12:00" },
    { "2014.09.04 15:00", "2014.09.04 18:00" },
    { "2014.09.05 09:00", "2014.09.05 12:00" },
  });

  TestRanges("Real world example: Was not processed right.", {
    "Mo off, Tu 14:00-18:00, We-Sa 10:00-18:00", // Reference value for prettify. Not perfect but still …
    "Mo geschl., Tu 14:00-18:00, We-Sa 10:00-18:00", // Reference value for prettify. Not perfect but still …
    "Mo: geschlossen, Di: 14-18Uhr, Mi-Sa: 10-18Uhr", // value as found in OSM
    // FIXME: See issue #50.
    "Mo off; Tu 14:00-18:00; We-Sa 10:00-18:00", // Please use this value instead. Mostly automatically corrected.
  }, "2014.01.06 0:00", "2014.01.13 0:00", {
    { "2014.01.07 14:00", "2014.01.07 18:00" },
    { "2014.01.08 10:00", "2014.01.08 18:00" },
    { "2014.01.09 10:00", "2014.01.09 18:00" },
    { "2014.01.10 10:00", "2014.01.10 18:00" },
    { "2014.01.11 10:00", "2014.01.11 18:00" },
  });

  TestRanges("Real world example: Was not processed right (month range/monthday range)", {
    "Aug,Dec 25-easter"
  }, "2014.01.01 0:00", "2015.01.01 0:00", {
    { "2014.01.01 00:00", "2014.04.20 00:00" },
    { "2014.08.01 00:00", "2014.09.01 00:00" },
    { "2014.12.25 00:00", "2015.01.01 00:00" },
  });

  // https://www.openstreetmap.org/node/2554317486
  TestRanges("Real world example: Was processed right (month range/monthday range with additional rule)", {
    "Nov-Mar Mo-Fr 11:30-17:00, Mo-Su 17:00-01:00"
  }, "2015.03.20 0:00", "2015.04.10 0:00", {
    { "2015.03.20 00:00", "2015.03.20 01:00" }, // Fr
    { "2015.03.20 11:30", "2015.03.21 01:00" }, // Fr
    { "2015.03.21 17:00", "2015.03.22 01:00" }, // Sa
    { "2015.03.22 17:00", "2015.03.23 01:00" }, // Su
    { "2015.03.23 11:30", "2015.03.24 01:00" }, // Mo
    { "2015.03.24 11:30", "2015.03.25 01:00" },
    { "2015.03.25 11:30", "2015.03.26 01:00" },
    { "2015.03.26 11:30", "2015.03.27 01:00" },
    { "2015.03.27 11:30", "2015.03.28 01:00" },
    { "2015.03.28 17:00", "2015.03.29 01:00" }, // Sa
    { "2015.03.29 17:00", "2015.03.30 01:00" }, // Su
    { "2015.03.30 11:30", "2015.03.31 01:00" }, // Mo
    { "2015.03.31 11:30", "2015.04.01 01:00" }, // Tu
    { "2015.04.01 17:00", "2015.04.02 01:00" }, // We
    { "2015.04.02 17:00", "2015.04.03 01:00" }, // Th
    { "2015.04.03 17:00", "2015.04.04 01:00" }, // Fr
    { "2015.04.04 17:00", "2015.04.05 01:00" }, // Sa
    { "2015.04.05 17:00", "2015.04.06 01:00" }, // Su
    { "2015.04.06 17:00", "2015.04.07 01:00" },
    { "2015.04.07 17:00", "2015.04.08 01:00" },
    { "2015.04.08 17:00", "2015.04.09 01:00" },
    { "2015.04.09 17:00", "2015.04.10 00:00" }, // Th
  });

  /* https://www.openstreetmap.org/node/1754337209/history {{{ */
  TestRanges("Real world example: Was not processed right (month range/monthday range)", {
    // "Jun 15-Sep 15: Th-Su 16:00-19:00; Sep 16-Dec 31: Sa,Su 16:00-19:00; Jan-Mar off; Dec 25-easter off"
    "Jun 15-Sep 15; Sep 16-Dec 31; Jan-Mar off; Dec 25-easter off"
  }, "2014.01.01 0:00", "2016.01.01 0:00", {
    { "2014.06.15 00:00", "2014.12.25 00:00" },
    { "2015.06.15 00:00", "2015.12.25 00:00" },
  });
  /* }}} */

  TestRanges("Real world example: Was not processed right", {
    "Mo, Tu, We, Th, Fr, Su 11:00-01:00; Sa 11:00-02:00",
  }, "2014.01.01 0:00", "2016.01.01 0:00", {
  });

  // problem with combined monthday and month selector {{{
  TestRanges("Real world example: Was not processed right.", {
    "Jan Su[-2]-Jan Su[-1]: Fr-Su 12:00+;"
    " Feb Su[-2]-Feb Su[-1]: Fr-Su 12:00+;"
    " Mar 1-Jul 31: Th-Su 12:00+;"
    " Aug 1-Nov 30,Dec: Tu-Su 12:00+;"
    " Dec 24-26,Dec 31: off", // Original value.
    "Jan Su[-2]-Jan Su[-1],Feb Su[-2]-Feb Su[-1]: Fr-Su 12:00+; Mar 1-Dec 31: Tu-Su 12:00+; Dec 24-26,Dec 31: off"
    // Optimized value. Should mean the same.
  }, "2014.11.29 0:00", "2015.01.11 0:00", {
    { "2014.11.29 12:00", "2014.11.30 00:00", open_end_comment, "true" },
    { "2014.11.30 12:00", "2014.12.01 00:00", open_end_comment, "true" },
    { "2014.12.02 12:00", "2014.12.03 00:00", open_end_comment, "true" },
    { "2014.12.03 12:00", "2014.12.04 00:00", open_end_comment, "true" },
    { "2014.12.04 12:00", "2014.12.05 00:00", open_end_comment, "true" },
    { "2014.12.05 12:00", "2014.12.06 00:00", open_end_comment, "true" },
    { "2014.12.06 12:00", "2014.12.07 00:00", open_end_comment, "true" },
    { "2014.12.07 12:00", "2014.12.08 00:00", open_end_comment, "true" },
    { "2014.12.09 12:00", "2014.12.10 00:00", open_end_comment, "true" },
    { "2014.12.10 12:00", "2014.12.11 00:00", open_end_comment, "true" },
    { "2014.12.11 12:00", "2014.12.12 00:00", open_end_comment, "true" },
    { "2014.12.12 12:00", "2014.12.13 00:00", open_end_comment, "true" },
    { "2014.12.13 12:00", "2014.12.14 00:00", open_end_comment, "true" },
    { "2014.12.14 12:00", "2014.12.15 00:00", open_end_comment, "true" },
    { "2014.12.16 12:00", "2014.12.17 00:00", open_end_comment, "true" },
    { "2014.12.17 12:00", "2014.12.18 00:00", open_end_comment, "true" },
    { "2014.12.18 12:00", "2014.12.19 00:00", open_end_comment, "true" },
    { "2014.12.19 12:00", "2014.12.20 00:00", open_end_comment, "true" },
    { "2014.12.20 12:00", "2014.12.21 00:00", open_end_comment, "true" },
    { "2014.12.21 12:00", "2014.12.22 00:00", open_end_comment, "true" },
    { "2014.12.23 12:00", "2014.12.24 00:00", open_end_comment, "true" },
    { "2014.12.27 12:00", "2014.12.28 00:00", open_end_comment, "true" },
    { "2014.12.28 12:00", "2014.12.29 00:00", open_end_comment, "true" },
    { "2014.12.30 12:00", "2014.12.31 00:00", open_end_comment, "true" },
  });

  TestRanges("Simplifed real world example: Was not processed right.", {
    "Nov 1-20,Dec",
  }, "2014.01.01 0:00", "2015.01.02 0:00", {
    { "2014.11.01 00:00", "2014.11.21 00:00" },
    { "2014.12.01 00:00", "2015.01.01 00:00" },
  });
  // }}}

  // https://github.com/ypid/opening_hours.js/issues/26 {{{
  // Problem with wrap day in browser.
  TestRanges("Real world example: Was processed right form library.", {
    "Mo 19:00+; We 14:00+; Su 10:00+ || \"Führung, Sonderführungen nach Vereinbarung.\"",
  }, "2014.01.06 0:00", "2014.01.13 0:00", {
    { "2014.01.06 00:00", "2014.01.06 19:00", "Führung, Sonderführungen nach Vereinbarung.", "true" },
    { "2014.01.06 19:00", "2014.01.07 05:00", open_end_comment, "true" },
    { "2014.01.07 05:00", "2014.01.08 14:00", "Führung, Sonderführungen nach Vereinbarung.", "true" },
    { "2014.01.08 14:00", "2014.01.09 00:00", open_end_comment, "true" },
    { "2014.01.09 00:00", "2014.01.12 10:00", "Führung, Sonderführungen nach Vereinbarung.", "true" },
    { "2014.01.12 10:00", "2014.01.13 00:00", open_end_comment, "true" },
  });

  TestRanges("Real world example: Was processed right form library.", {
    "Mo 19:00-05:00 || \"Sonderführungen nach Vereinbarung.\"",
  }, "2014.01.06 0:00", "2014.01.13 0:00", {
    { "2014.01.06 00:00", "2014.01.06 19:00", "Sonderführungen nach Vereinbarung.", "true" },
    { "2014.01.06 19:00", "2014.01.07 05:00" },
    { "2014.01.07 05:00", "2014.01.13 00:00", "Sonderführungen nach Vereinbarung.", "true" },
  });

  TestRanges("Real world example: Was processed right form library.", {
    "Mo 19:00+ || \"Sonderführungen nach Vereinbarung.\"",
  }, "2014.01.07 1:00", "2014.01.13 0:00", {
    { "2014.01.07 01:00", "2014.01.07 05:00", open_end_comment, "true" },
    { "2014.01.07 05:00", "2014.01.13 00:00", "Sonderführungen nach Vereinbarung.", "true" },
  });
  // }}}

  // https://github.com/ypid/opening_hours.js/issues/27 {{{
  // Problem in browser.
  //
  // https://www.openstreetmap.org/way/163756418/history
  TestRanges("Real world example: Was not processed right.", {
    "Jun 15-Sep 15: Th-Su 16:00-19:00; Sep 16-Dec 31: Sa,Su 16:00-19:00; Jan,Feb,Mar off; Dec 25,easter off",
  }, "2013.12.20 0:00", "2014.06.20 0:00", {
    { "2013.12.21 16:00", "2013.12.21 19:00" }, // Sa
    { "2013.12.22 16:00", "2013.12.22 19:00" }, // Su
    { "2013.12.28 16:00", "2013.12.28 19:00" }, // Sa
    { "2013.12.29 16:00", "2013.12.29 19:00" }, // Su
    { "2014.06.15 16:00", "2014.06.15 19:00" }, // Su
    { "2014.06.19 16:00", "2014.06.19 19:00" }, // Th
  });

  TestRanges("Based on real world example: Is processed right.", {
    "Nov-Dec Sa,Su 16:00-19:00; Dec 22 off",
  }, "2013.01.01 0:00", "2014.01.01 0:00", {
    { "2013.11.02 16:00", "2013.11.02 19:00" },
    { "2013.11.03 16:00", "2013.11.03 19:00" },
    { "2013.11.09 16:00", "2013.11.09 19:00" },
    { "2013.11.10 16:00", "2013.11.10 19:00" },
    { "2013.11.16 16:00", "2013.11.16 19:00" },
    { "2013.11.17 16:00", "2013.11.17 19:00" },
    { "2013.11.23 16:00", "2013.11.23 19:00" },
    { "2013.11.24 16:00", "2013.11.24 19:00" },
    { "2013.11.30 16:00", "2013.11.30 19:00" },
    { "2013.12.01 16:00", "2013.12.01 19:00" },
    { "2013.12.07 16:00", "2013.12.07 19:00" },
    { "2013.12.08 16:00", "2013.12.08 19:00" },
    { "2013.12.14 16:00", "2013.12.14 19:00" },
    { "2013.12.15 16:00", "2013.12.15 19:00" },
    { "2013.12.21 16:00", "2013.12.21 19:00" },
    { "2013.12.28 16:00", "2013.12.28 19:00" },
    { "2013.12.29 16:00", "2013.12.29 19:00" },
  });

  TestRanges("Based on real world example: Is processed right.", {
    "May-Sep: 00:00-24:00, Apr-Oct: Sa-Su 08:00-15:00",
    "Apr-Oct: Sa-Su 08:00-15:00, May-Sep: 00:00-24:00",
    "Apr-Oct: Sa-Su 08:00-15:00; May-Sep: 00:00-24:00",
  }, "2013.01.01 0:00", "2014.01.01 0:00", {
    { "2013.04.06 08:00", "2013.04.06 15:00" }, // 8 days: Apr {{{
    { "2013.04.07 08:00", "2013.04.07 15:00" },
    { "2013.04.13 08:00", "2013.04.13 15:00" },
    { "2013.04.14 08:00", "2013.04.14 15:00" },
    { "2013.04.20 08:00", "2013.04.20 15:00" },
    { "2013.04.21 08:00", "2013.04.21 15:00" },
    { "2013.04.27 08:00", "2013.04.27 15:00" },
    { "2013.04.28 08:00", "2013.04.28 15:00" }, // }}}
    { "2013.05.01 00:00", "2013.10.01 00:00" }, // 31 + 30 + 31 + 31 + 30 days: May-Sep
    { "2013.10.05 08:00", "2013.10.05 15:00" }, // 8 days: Oct {{{
    { "2013.10.06 08:00", "2013.10.06 15:00" },
    { "2013.10.12 08:00", "2013.10.12 15:00" },
    { "2013.10.13 08:00", "2013.10.13 15:00" },
    { "2013.10.19 08:00", "2013.10.19 15:00" },
    { "2013.10.20 08:00", "2013.10.20 15:00" },
    { "2013.10.26 08:00", "2013.10.26 15:00" },
    { "2013.10.27 08:00", "2013.10.27 15:00" }, // }}}
  });
  // }}}

  // https://github.com/ypid/opening_hours.js/issues/43 {{{
  TestRanges("Real world example: Was not processed right.", {
    "Mo-Fr 07:00-19:30; Sa-Su 08:00-19:30; 19:30-21:00 open \"No new laundry loads in\"; Nov Th[4] off; Dec 25 off",
  }, "2014.12.23 0:00", "2014.12.27 0:00", {
    { "2014.12.23 07:00", "2014.12.23 19:30" }, // Tu
    { "2014.12.23 19:30", "2014.12.23 21:00", "No new laundry loads in" },
    { "2014.12.24 07:00", "2014.12.24 19:30" }, // We
    { "2014.12.24 19:30", "2014.12.24 21:00", "No new laundry loads in" },
    { "2014.12.26 07:00", "2014.12.26 19:30" }, // Fr
    { "2014.12.26 19:30", "2014.12.26 21:00", "No new laundry loads in" },
  });
  // }}}

  /* https://www.openstreetmap.org/node/35608651/history {{{ */
  TestRanges("Real world example: Was not processed right", {
    "Jan off; Feb off; Mar off; Apr Tu-Su 10:00-14:30, May Tu-Su 10:00-14:30; Jun Tu-Su 09:00-16:00; Jul Tu-Su 10:00-17:00; Aug Tu-Su 10:00-17:00; Sep Tu-Su 10:00-14:30; Oct Tu-Su 10:00-14:30 Nov off; Dec off", // FIXME
    "Nov-Mar off; Apr,May,Sep,Oct Tu-Su 10:00-14:30; Jun Tu-Su 09:00-16:00; Jul,Aug Tu-Su 10:00-17:00",
    "Nov-Mar off; Apr,May,Sep,Oct 10:00-14:30; Jun 09:00-16:00; Jul,Aug 10:00-17:00; Mo off"
  }, "2014.03.15 0:00", "2014.05.02 0:00", {
    { "2014.04.01 10:00", "2014.04.01 14:30" },
    { "2014.04.02 10:00", "2014.04.02 14:30" },
    { "2014.04.03 10:00", "2014.04.03 14:30" },
    { "2014.04.04 10:00", "2014.04.04 14:30" },
    { "2014.04.05 10:00", "2014.04.05 14:30" },
    { "2014.04.06 10:00", "2014.04.06 14:30" },
    { "2014.04.08 10:00", "2014.04.08 14:30" },
    { "2014.04.09 10:00", "2014.04.09 14:30" },
    { "2014.04.10 10:00", "2014.04.10 14:30" },
    { "2014.04.11 10:00", "2014.04.11 14:30" },
    { "2014.04.12 10:00", "2014.04.12 14:30" },
    { "2014.04.13 10:00", "2014.04.13 14:30" },
    { "2014.04.15 10:00", "2014.04.15 14:30" },
    { "2014.04.16 10:00", "2014.04.16 14:30" },
    { "2014.04.17 10:00", "2014.04.17 14:30" },
    { "2014.04.18 10:00", "2014.04.18 14:30" },
    { "2014.04.19 10:00", "2014.04.19 14:30" },
    { "2014.04.20 10:00", "2014.04.20 14:30" },
    { "2014.04.22 10:00", "2014.04.22 14:30" },
    { "2014.04.23 10:00", "2014.04.23 14:30" },
    { "2014.04.24 10:00", "2014.04.24 14:30" },
    { "2014.04.25 10:00", "2014.04.25 14:30" },
    { "2014.04.26 10:00", "2014.04.26 14:30" },
    { "2014.04.27 10:00", "2014.04.27 14:30" },
    { "2014.04.29 10:00", "2014.04.29 14:30" },
    { "2014.04.30 10:00", "2014.04.30 14:30" },
    { "2014.05.01 10:00", "2014.05.01 14:30" },
  });

  /* {{{ Test over a full year */
  TestRanges("Real world example: Was not processed right (test over a full year)", {
    "Jan off; Feb off; Mar off; Apr Tu-Su 10:00-14:30, May Tu-Su 10:00-14:30; Jun Tu-Su 09:00-16:00; Jul Tu-Su 10:00-17:00; Aug Tu-Su 10:00-17:00; Sep Tu-Su 10:00-14:30; Oct Tu-Su 10:00-14:30; Nov off; Dec off",
    "Nov-Mar off; Apr,May,Sep,Oct Tu-Su 10:00-14:30; Jun Tu-Su 09:00-16:00; Jul,Aug Tu-Su 10:00-17:00",
    "Nov-Mar off; Apr,May,Sep,Oct 10:00-14:30; Jun 09:00-16:00; Jul,Aug 10:00-17:00; Mo off"
  }, "2014.03.15 0:00", "2015.05.02 0:00", {
    { "2014.04.01 10:00", "2014.04.01 14:30" },
    { "2014.04.02 10:00", "2014.04.02 14:30" },
    { "2014.04.03 10:00", "2014.04.03 14:30" },
    { "2014.04.04 10:00", "2014.04.04 14:30" },
    { "2014.04.05 10:00", "2014.04.05 14:30" },
    { "2014.04.06 10:00", "2014.04.06 14:30" },
    { "2014.04.08 10:00", "2014.04.08 14:30" },
    { "2014.04.09 10:00", "2014.04.09 14:30" },
    { "2014.04.10 10:00", "2014.04.10 14:30" },
    { "2014.04.11 10:00", "2014.04.11 14:30" },
    { "2014.04.12 10:00", "2014.04.12 14:30" },
    { "2014.04.13 10:00", "2014.04.13 14:30" },
    { "2014.04.15 10:00", "2014.04.15 14:30" },
    { "2014.04.16 10:00", "2014.04.16 14:30" },
    { "2014.04.17 10:00", "2014.04.17 14:30" },
    { "2014.04.18 10:00", "2014.04.18 14:30" },
    { "2014.04.19 10:00", "2014.04.19 14:30" },
    { "2014.04.20 10:00", "2014.04.20 14:30" },
    { "2014.04.22 10:00", "2014.04.22 14:30" },
    { "2014.04.23 10:00", "2014.04.23 14:30" },
    { "2014.04.24 10:00", "2014.04.24 14:30" },
    { "2014.04.25 10:00", "2014.04.25 14:30" },
    { "2014.04.26 10:00", "2014.04.26 14:30" },
    { "2014.04.27 10:00", "2014.04.27 14:30" },
    { "2014.04.29 10:00", "2014.04.29 14:30" },
    { "2014.04.30 10:00", "2014.04.30 14:30" },
    { "2014.05.01 10:00", "2014.05.01 14:30" },
    { "2014.05.02 10:00", "2014.05.02 14:30" },
    { "2014.05.03 10:00", "2014.05.03 14:30" },
    { "2014.05.04 10:00", "2014.05.04 14:30" },
    { "2014.05.06 10:00", "2014.05.06 14:30" },
    { "2014.05.07 10:00", "2014.05.07 14:30" },
    { "2014.05.08 10:00", "2014.05.08 14:30" },
    { "2014.05.09 10:00", "2014.05.09 14:30" },
    { "2014.05.10 10:00", "2014.05.10 14:30" },
    { "2014.05.11 10:00", "2014.05.11 14:30" },
    { "2014.05.13 10:00", "2014.05.13 14:30" },
    { "2014.05.14 10:00", "2014.05.14 14:30" },
    { "2014.05.15 10:00", "2014.05.15 14:30" },
    { "2014.05.16 10:00", "2014.05.16 14:30" },
    { "2014.05.17 10:00", "2014.05.17 14:30" },
    { "2014.05.18 10:00", "2014.05.18 14:30" },
    { "2014.05.20 10:00", "2014.05.20 14:30" },
    { "2014.05.21 10:00", "2014.05.21 14:30" },
    { "2014.05.22 10:00", "2014.05.22 14:30" },
    { "2014.05.23 10:00", "2014.05.23 14:30" },
    { "2014.05.24 10:00", "2014.05.24 14:30" },
    { "2014.05.25 10:00", "2014.05.25 14:30" },
    { "2014.05.27 10:00", "2014.05.27 14:30" },
    { "2014.05.28 10:00", "2014.05.28 14:30" },
    { "2014.05.29 10:00", "2014.05.29 14:30" },
    { "2014.05.30 10:00", "2014.05.30 14:30" },
    { "2014.05.31 10:00", "2014.05.31 14:30" },
    { "2014.06.01 09:00", "2014.06.01 16:00" },
    { "2014.06.03 09:00", "2014.06.03 16:00" },
    { "2014.06.04 09:00", "2014.06.04 16:00" },
    { "2014.06.05 09:00", "2014.06.05 16:00" },
    { "2014.06.06 09:00", "2014.06.06 16:00" },
    { "2014.06.07 09:00", "2014.06.07 16:00" },
    { "2014.06.08 09:00", "2014.06.08 16:00" },
    { "2014.06.10 09:00", "2014.06.10 16:00" },
    { "2014.06.11 09:00", "2014.06.11 16:00" },
    { "2014.06.12 09:00", "2014.06.12 16:00" },
    { "2014.06.13 09:00", "2014.06.13 16:00" },
    { "2014.06.14 09:00", "2014.06.14 16:00" },
    { "2014.06.15 09:00", "2014.06.15 16:00" },
    { "2014.06.17 09:00", "2014.06.17 16:00" },
    { "2014.06.18 09:00", "2014.06.18 16:00" },
    { "2014.06.19 09:00", "2014.06.19 16:00" },
    { "2014.06.20 09:00", "2014.06.20 16:00" },
    { "2014.06.21 09:00", "2014.06.21 16:00" },
    { "2014.06.22 09:00", "2014.06.22 16:00" },
    { "2014.06.24 09:00", "2014.06.24 16:00" },
    { "2014.06.25 09:00", "2014.06.25 16:00" },
    { "2014.06.26 09:00", "2014.06.26 16:00" },
    { "2014.06.27 09:00", "2014.06.27 16:00" },
    { "2014.06.28 09:00", "2014.06.28 16:00" },
    { "2014.06.29 09:00", "2014.06.29 16:00" },
    { "2014.07.01 10:00", "2014.07.01 17:00" },
    { "2014.07.02 10:00", "2014.07.02 17:00" },
    { "2014.07.03 10:00", "2014.07.03 17:00" },
    { "2014.07.04 10:00", "2014.07.04 17:00" },
    { "2014.07.05 10:00", "2014.07.05 17:00" },
    { "2014.07.06 10:00", "2014.07.06 17:00" },
    { "2014.07.08 10:00", "2014.07.08 17:00" },
    { "2014.07.09 10:00", "2014.07.09 17:00" },
    { "2014.07.10 10:00", "2014.07.10 17:00" },
    { "2014.07.11 10:00", "2014.07.11 17:00" },
    { "2014.07.12 10:00", "2014.07.12 17:00" },
    { "2014.07.13 10:00", "2014.07.13 17:00" },
    { "2014.07.15 10:00", "2014.07.15 17:00" },
    { "2014.07.16 10:00", "2014.07.16 17:00" },
    { "2014.07.17 10:00", "2014.07.17 17:00" },
    { "2014.07.18 10:00", "2014.07.18 17:00" },
    { "2014.07.19 10:00", "2014.07.19 17:00" },
    { "2014.07.20 10:00", "2014.07.20 17:00" },
    { "2014.07.22 10:00", "2014.07.22 17:00" },
    { "2014.07.23 10:00", "2014.07.23 17:00" },
    { "2014.07.24 10:00", "2014.07.24 17:00" },
    { "2014.07.25 10:00", "2014.07.25 17:00" },
    { "2014.07.26 10:00", "2014.07.26 17:00" },
    { "2014.07.27 10:00", "2014.07.27 17:00" },
    { "2014.07.29 10:00", "2014.07.29 17:00" },
    { "2014.07.30 10:00", "2014.07.30 17:00" },
    { "2014.07.31 10:00", "2014.07.31 17:00" },
    { "2014.08.01 10:00", "2014.08.01 17:00" },
    { "2014.08.02 10:00", "2014.08.02 17:00" },
    { "2014.08.03 10:00", "2014.08.03 17:00" },
    { "2014.08.05 10:00", "2014.08.05 17:00" },
    { "2014.08.06 10:00", "2014.08.06 17:00" },
    { "2014.08.07 10:00", "2014.08.07 17:00" },
    { "2014.08.08 10:00", "2014.08.08 17:00" },
    { "2014.08.09 10:00", "2014.08.09 17:00" },
    { "2014.08.10 10:00", "2014.08.10 17:00" },
    { "2014.08.12 10:00", "2014.08.12 17:00" },
    { "2014.08.13 10:00", "2014.08.13 17:00" },
    { "2014.08.14 10:00", "2014.08.14 17:00" },
    { "2014.08.15 10:00", "2014.08.15 17:00" },
    { "2014.08.16 10:00", "2014.08.16 17:00" },
    { "2014.08.17 10:00", "2014.08.17 17:00" },
    { "2014.08.19 10:00", "2014.08.19 17:00" },
    { "2014.08.20 10:00", "2014.08.20 17:00" },
    { "2014.08.21 10:00", "2014.08.21 17:00" },
    { "2014.08.22 10:00", "2014.08.22 17:00" },
    { "2014.08.23 10:00", "2014.08.23 17:00" },
    { "2014.08.24 10:00", "2014.08.24 17:00" },
    { "2014.08.26 10:00", "2014.08.26 17:00" },
    { "2014.08.27 10:00", "2014.08.27 17:00" },
    { "2014.08.28 10:00", "2014.08.28 17:00" },
    { "2014.08.29 10:00", "2014.08.29 17:00" },
    { "2014.08.30 10:00", "2014.08.30 17:00" },
    { "2014.08.31 10:00", "2014.08.31 17:00" },
    { "2014.09.02 10:00", "2014.09.02 14:30" },
    { "2014.09.03 10:00", "2014.09.03 14:30" },
    { "2014.09.04 10:00", "2014.09.04 14:30" },
    { "2014.09.05 10:00", "2014.09.05 14:30" },
    { "2014.09.06 10:00", "2014.09.06 14:30" },
    { "2014.09.07 10:00", "2014.09.07 14:30" },
    { "2014.09.09 10:00", "2014.09.09 14:30" },
    { "2014.09.10 10:00", "2014.09.10 14:30" },
    { "2014.09.11 10:00", "2014.09.11 14:30" },
    { "2014.09.12 10:00", "2014.09.12 14:30" },
    { "2014.09.13 10:00", "2014.09.13 14:30" },
    { "2014.09.14 10:00", "2014.09.14 14:30" },
    { "2014.09.16 10:00", "2014.09.16 14:30" },
    { "2014.09.17 10:00", "2014.09.17 14:30" },
    { "2014.09.18 10:00", "2014.09.18 14:30" },
    { "2014.09.19 10:00", "2014.09.19 14:30" },
    { "2014.09.20 10:00", "2014.09.20 14:30" },
    { "2014.09.21 10:00", "2014.09.21 14:30" },
    { "2014.09.23 10:00", "2014.09.23 14:30" },
    { "2014.09.24 10:00", "2014.09.24 14:30" },
    { "2014.09.25 10:00", "2014.09.25 14:30" },
    { "2014.09.26 10:00", "2014.09.26 14:30" },
    { "2014.09.27 10:00", "2014.09.27 14:30" },
    { "2014.09.28 10:00", "2014.09.28 14:30" },
    { "2014.09.30 10:00", "2014.09.30 14:30" },
    { "2014.10.01 10:00", "2014.10.01 14:30" },
    { "2014.10.02 10:00", "2014.10.02 14:30" },
    { "2014.10.03 10:00", "2014.10.03 14:30" },
    { "2014.10.04 10:00", "2014.10.04 14:30" },
    { "2014.10.05 10:00", "2014.10.05 14:30" },
    { "2014.10.07 10:00", "2014.10.07 14:30" },
    { "2014.10.08 10:00", "2014.10.08 14:30" },
    { "2014.10.09 10:00", "2014.10.09 14:30" },
    { "2014.10.10 10:00", "2014.10.10 14:30" },
    { "2014.10.11 10:00", "2014.10.11 14:30" },
    { "2014.10.12 10:00", "2014.10.12 14:30" },
    { "2014.10.14 10:00", "2014.10.14 14:30" },
    { "2014.10.15 10:00", "2014.10.15 14:30" },
    { "2014.10.16 10:00", "2014.10.16 14:30" },
    { "2014.10.17 10:00", "2014.10.17 14:30" },
    { "2014.10.18 10:00", "2014.10.18 14:30" },
    { "2014.10.19 10:00", "2014.10.19 14:30" },
    { "2014.10.21 10:00", "2014.10.21 14:30" },
    { "2014.10.22 10:00", "2014.10.22 14:30" },
    { "2014.10.23 10:00", "2014.10.23 14:30" },
    { "2014.10.24 10:00", "2014.10.24 14:30" },
    { "2014.10.25 10:00", "2014.10.25 14:30" },
    { "2014.10.26 10:00", "2014.10.26 14:30" },
    { "2014.10.28 10:00", "2014.10.28 14:30" },
    { "2014.10.29 10:00", "2014.10.29 14:30" },
    { "2014.10.30 10:00", "2014.10.30 14:30" },
    { "2014.10.31 10:00", "2014.10.31 14:30" },
    { "2015.04.01 10:00", "2015.04.01 14:30" },
    { "2015.04.02 10:00", "2015.04.02 14:30" },
    { "2015.04.03 10:00", "2015.04.03 14:30" },
    { "2015.04.04 10:00", "2015.04.04 14:30" },
    { "2015.04.05 10:00", "2015.04.05 14:30" },
    { "2015.04.07 10:00", "2015.04.07 14:30" },
    { "2015.04.08 10:00", "2015.04.08 14:30" },
    { "2015.04.09 10:00", "2015.04.09 14:30" },
    { "2015.04.10 10:00", "2015.04.10 14:30" },
    { "2015.04.11 10:00", "2015.04.11 14:30" },
    { "2015.04.12 10:00", "2015.04.12 14:30" },
    { "2015.04.14 10:00", "2015.04.14 14:30" },
    { "2015.04.15 10:00", "2015.04.15 14:30" },
    { "2015.04.16 10:00", "2015.04.16 14:30" },
    { "2015.04.17 10:00", "2015.04.17 14:30" },
    { "2015.04.18 10:00", "2015.04.18 14:30" },
    { "2015.04.19 10:00", "2015.04.19 14:30" },
    { "2015.04.21 10:00", "2015.04.21 14:30" },
    { "2015.04.22 10:00", "2015.04.22 14:30" },
    { "2015.04.23 10:00", "2015.04.23 14:30" },
    { "2015.04.24 10:00", "2015.04.24 14:30" },
    { "2015.04.25 10:00", "2015.04.25 14:30" },
    { "2015.04.26 10:00", "2015.04.26 14:30" },
    { "2015.04.28 10:00", "2015.04.28 14:30" },
    { "2015.04.29 10:00", "2015.04.29 14:30" },
    { "2015.04.30 10:00", "2015.04.30 14:30" },
    { "2015.05.01 10:00", "2015.05.01 14:30" },
  });
  // }}}
  // }}}

  TestRanges("Real world example: Was not processed right", {
    "Tu 10:00-12:00, Fr 16:00-18:00; unknown",
  }, "2014.01.01 0:00", "2016.01.01 0:00", {
  });

  /* {{{ https://www.openstreetmap.org/node/3010451545 */
  TestShouldFail("Incorrect syntax which should throw an error", {
    "MON-FRI 5PM-12AM | SAT-SUN 12PM-12AM",  // Website.
    "MON-FRI 5PM-12AM; SAT-SUN 12PM-12AM",   // Website, using valid <normal_rule_separator>.
    "Mo-Fr 17:00-12:00; Sa-Su 24:00-12:00",  // Website, after cleanup and am/pm to normal time conversion.
  });

  TestRanges("Real world example: Was not processed right", {
    "Mo-Fr 17:00-12:00, Su-Mo 00:00-12:00", // Rewritten and fixed.
  }, "2014.08.25 0:00", "2014.09.02 0:00", {
    { "2014.08.25 00:00", "2014.08.25 12:00" }, // Mo
    { "2014.08.25 17:00", "2014.08.26 12:00" }, // Mo to Tu
    { "2014.08.26 17:00", "2014.08.27 12:00" }, // Tu to We
    { "2014.08.27 17:00", "2014.08.28 12:00" }, // We to Th
    { "2014.08.28 17:00", "2014.08.29 12:00" }, // Th to Fr
    { "2014.08.29 17:00", "2014.08.30 12:00" }, // Fr to Sa
    { "2014.08.31 00:00", "2014.08.31 12:00" }, // (Sa to) Su
    { "2014.09.01 00:00", "2014.09.01 12:00" }, // (Su to) Mo
    { "2014.09.01 17:00", "2014.09.02 00:00" }, // Mo to Tu
  });
  // }}}

  // }}}

  // additional rules {{{

  // for https://github.com/ypid/opening_hours.js/issues/16
  TestRanges("Additional rules with comment", {
    "Fr 08:00-12:00, Fr 12:00-16:00 open \"Notfallsprechstunde\"",
    "Fr 08:00-12:00 || Fr 12:00-16:00 open \"Notfallsprechstunde\"", // should mean the same
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.05 08:00", "2012.10.05 12:00" },
    { "2012.10.05 12:00", "2012.10.05 16:00", "Notfallsprechstunde" },
  });
  // }}}

  // period times {{{
  TestRanges("Points in time, period times", {
    "Mo-Fr 10:00-16:00/01:30",
    "Mo-Fr 10:00-16:00/90",
    "Mo-Fr 10:00-16:00/90; Sa off \"testing at end for parser\"",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 10:00", "2012.10.01 10:01" },
    { "2012.10.01 11:30", "2012.10.01 11:31" },
    { "2012.10.01 13:00", "2012.10.01 13:01" },
    { "2012.10.01 14:30", "2012.10.01 14:31" },
    { "2012.10.01 16:00", "2012.10.01 16:01" },
    { "2012.10.02 10:00", "2012.10.02 10:01" },
    { "2012.10.02 11:30", "2012.10.02 11:31" },
    { "2012.10.02 13:00", "2012.10.02 13:01" },
    { "2012.10.02 14:30", "2012.10.02 14:31" },
    { "2012.10.02 16:00", "2012.10.02 16:01" },
  });

  TestRanges("Points in time, period times", {
    "Mo-Fr 10:00-16:00/02:00",
    "Mo-Fr 10:00-16:00/120",
  }, "2012.10.01 0:00", "2012.10.02 0:00", {
    { "2012.10.01 10:00", "2012.10.01 10:01" },
    { "2012.10.01 12:00", "2012.10.01 12:01" },
    { "2012.10.01 14:00", "2012.10.01 14:01" },
    { "2012.10.01 16:00", "2012.10.01 16:01" },
  });

  TestRanges("Points in time, period times time wrap", {
    "Mo-Fr 22:00-03:00/01:00",
  }, "2012.10.01 0:00", "2012.10.03 0:00", {
    { "2012.10.01 22:00", "2012.10.01 22:01" },
    { "2012.10.01 23:00", "2012.10.01 23:01" },
    { "2012.10.02 00:00", "2012.10.02 00:01" },
    { "2012.10.02 01:00", "2012.10.02 01:01" },
    { "2012.10.02 02:00", "2012.10.02 02:01" },
    { "2012.10.02 03:00", "2012.10.02 03:01" },
    { "2012.10.02 22:00", "2012.10.02 22:01" },
    { "2012.10.02 23:00", "2012.10.02 23:01" },
  });

  // FIXME
  TestRanges("Points in time, period times (real world example)", {
    "Sa 08:00,09:00,10:00,11:00,12:00,13:00,14:00, Mo-Fr 15:00,16:00,17:00,18:00,19:00,20:00",
    "Mo-Fr 15:00-20:00/60; Sa 08:00-14:00/60", // Preferred because shorter and easier to read and maintain.
  }, "2013.12.06 0:00", "2013.12.08 0:00", {
    { "2013.12.06 15:00", "2013.12.06 15:01" },
    { "2013.12.06 16:00", "2013.12.06 16:01" },
    { "2013.12.06 17:00", "2013.12.06 17:01" },
    { "2013.12.06 18:00", "2013.12.06 18:01" },
    { "2013.12.06 19:00", "2013.12.06 19:01" },
    { "2013.12.06 20:00", "2013.12.06 20:01" },
    { "2013.12.07 08:00", "2013.12.07 08:01" },
    { "2013.12.07 09:00", "2013.12.07 09:01" },
    { "2013.12.07 10:00", "2013.12.07 10:01" },
    { "2013.12.07 11:00", "2013.12.07 11:01" },
    { "2013.12.07 12:00", "2013.12.07 12:01" },
    { "2013.12.07 13:00", "2013.12.07 13:01" },
    { "2013.12.07 14:00", "2013.12.07 14:01" },
  });
  // }}}
  // }}}

  // error tolerance {{{
  TestRanges("Error tolerance: case and whitespace", {
    "Mo,Tu,We,Th 12:00-20:00; 14:00-16:00 off", // reference value for prettify
    "   monday,    Tu, wE,   TH    12:00 - 20:00  ; 14:00-16:00	Off  ",
    "   monday,    Tu, wE,   TH    12:00 - 20:00  ; Off 14:00-16:00	", // Warnings point to the wrong position for selector reorder.
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 12:00", "2012.10.01 14:00" },
    { "2012.10.01 16:00", "2012.10.01 20:00" },
    { "2012.10.02 12:00", "2012.10.02 14:00" },
    { "2012.10.02 16:00", "2012.10.02 20:00" },
    { "2012.10.03 12:00", "2012.10.03 14:00" },
    { "2012.10.03 16:00", "2012.10.03 20:00" },
    { "2012.10.04 12:00", "2012.10.04 14:00" },
    { "2012.10.04 16:00", "2012.10.04 20:00" },
  });

  TestRanges("Error tolerance: weekdays, months in different languages", {
    "Mo,Tu,We,Th 12:00-20:00; 14:00-16:00 off", // reference value for prettify
    "mon, Dienstag, Mi, donnerstag 12:00-20:00; 14:00-16:00 off",
    "mon, Tuesday, wed, Thursday 12:00-20:00; 14:00-16:00 off",
    "mon., Tuesday, wed., Thursday. 12:00-20:00; 14:00-16:00 off",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 12:00", "2012.10.01 14:00" },
    { "2012.10.01 16:00", "2012.10.01 20:00" },
    { "2012.10.02 12:00", "2012.10.02 14:00" },
    { "2012.10.02 16:00", "2012.10.02 20:00" },
    { "2012.10.03 12:00", "2012.10.03 14:00" },
    { "2012.10.03 16:00", "2012.10.03 20:00" },
    { "2012.10.04 12:00", "2012.10.04 14:00" },
    { "2012.10.04 16:00", "2012.10.04 20:00" },
  });

  TestRanges("Error tolerance: Full range", {
    "Mo-Su",       // reference value for prettify
    "Montag-Sonntag",
    "Montags bis sonntags",       // Do not use. Returns warning.
    "Montag-Sonntags",
    "monday-sunday",
    "daily",
    "everyday",
    "every day",
    "all days",
    "every day",
    "7days",
    "7j/7",
    "7/7",
    "7 days",
    "7 days a week",
    "7 days/week",
    "täglich",
    "week 1-53",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 0:00", "2012.10.08 0:00" },
  });

  TestRanges("Error tolerance: Full range", {
    "24/7",       // reference value for prettify
    "always",
    "always open",
    "nonstop",
    "nonstop geöffnet",
    "opening_hours=nonstop geöffnet",
    "opening_hours =nonstop geöffnet",
    "opening_hours 	 =nonstop geöffnet",
    "opening_hours = nonstop geöffnet",
    "Öffnungszeit nonstop geöffnet",
    "Öffnungszeit: nonstop geöffnet",
    "Öffnungszeiten nonstop geöffnet",
    "Öffnungszeiten: nonstop geöffnet",
    "24x7",
    "anytime",
    "all day",
    "24 hours 7 days a week",
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 0:00", "2012.10.08 0:00" },
  });
  // }}}

  // values which should return a warning {{{
  TestRanges("Extensions: missing time range separators", {
    "Mo 12:00-14:00 16:00-18:00 20:00-22:00", // returns a warning
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 12:00", "2012.10.01 14:00" },
    { "2012.10.01 16:00", "2012.10.01 18:00" },
    { "2012.10.01 20:00", "2012.10.01 22:00" },
  });

  TestRanges("Time intervals (not specified/documented use of colon, please avoid this)", {
    "00:00-24:00; Mo 15:00-16:00 off",  // prettified value
    "00:00-24:00; Mo: 15:00-16:00 off", // The colon between weekday and time range is ignored. This is used in OSM.
  }, "2012.10.01 0:00", "2012.10.08 0:00", {
    { "2012.10.01 00:00", "2012.10.01 15:00" },
    { "2012.10.01 16:00", "2012.10.08 00:00" },
  });

  // values which should fail during parsing {{{
  TestShouldFail("Incorrect syntax which should throw an error", {
    // stupid tests {{{
    "sdasdlasdj a3reaw", // Test for the test framwork. This test should pass :) (passes when the value can not be parsed)
    "", // empty string
    " ", // empty string
    "\n", // newline
    ";", // only rule delimiter
    "||", // only rule delimiter
    // "12:00-14:00 ||",
    // }}}
    "Mo[2] - 7 days" + value_suffix,
    ":week 2-54 00:00-24:00" + value_suffix,
    ":::week 2-54 00:00-24:00" + value_suffix,
    "week :2-54 00:00-24:00" + value_suffix,
    "week week",
    "week week 5",
    "week 0",
    "week 54",
    "week 1-54",
    "week 0-54",
    "week 40-30",
    "week 30-40/1",
    "week 30-40/27",
    "week week 00:00-24:00" + value_suffix,
    "week 2-53 00:00-24:00:" + value_suffix,
    "week 2-53 00:00-24:00:::" + value_suffix,
    "week 2-53 00::00-24:00" + value_suffix,
    "week 2-52/2 We, week 1-53/2 Sa 0:00-24:00" + value_suffix, // See definition of fallback rules in the README.md: *additional rules*
    "(sunrise+01:00-sunset" + value_suffix,
    "(sunrise+01::)-sunset" + value_suffix,
    "(sunrise)-sunset" + value_suffix,
    "(" + value_suffix,
    "sunrise-(" + value_suffix,
    "sunrise-sunset,(" + value_suffix,
    "dusk;dawn" + value_suffix,
    "dusk" + value_suffix,
    "27:00-29:00" + value_suffix,
    "14:/" + value_suffix,
    "14:00/" + value_suffix,
    "14:00-/" + value_suffix,
    "14:00-16:00,." + value_suffix,
    "11" + value_suffix,
    "11am" + value_suffix,
    "14:00-16:00,11:00" + value_suffix,
    // "14:00-16:00,", // is ok
    "21:00-22:60" + value_suffix,
    "21:60-22:59" + value_suffix,
    "Sa[1." + value_suffix,
    "Sa[1,0,3]" + value_suffix,
    "Sa[1,3-6]" + value_suffix,
    "Sa[1,3-.]" + value_suffix,
    "Sa[1,3,.]" + value_suffix,
    "PH + 2 day" + value_suffix, // Normally moving PH one day is everything you will need. Handling more than one move day would be harder to implement correctly.
    "Su-PH" + value_suffix,      // not accepted syntax
    "2012, Jan" + value_suffix,
    "easter + 370 days" + value_suffix,
    "easter - 2 days - 2012 easter + 2 days: open \"Easter Monday\"" + value_suffix,
    "2012 easter - 2 days - easter + 2 days: open \"Easter Monday\"" + value_suffix,
    // "easter + 198 days", // Does throw an error, but at runtime when the problem occurs.
    "Jan,,,Dec" + value_suffix,
    "Mo,,Th" + value_suffix,
    "12:00-15:00/60" + value_suffix,
    "12:00-15:00/1:00" + value_suffix,
    "12:00-15:00/1:" + value_suffix,
    "Jun 0-Aug 23" + value_suffix, // out of range
    "Feb 30-Aug 2" + value_suffix, // out of range
    "Jun 2-Aug 42" + value_suffix, // out of range
    "Jun 2-Aug 32" + value_suffix, // out of range
    "Jun 2-32" + value_suffix,     // out of range
    "Jun 32-34" + value_suffix,    // out of range
    "Jun 2-32/2" + value_suffix,   // out of range
    "Jun 32" + value_suffix,       // out of range
    "Jun 30-24" + value_suffix,    // reverse
    "Jun 2-20/0" + value_suffix,   // period is zero
    "2014-2020/0" + value_suffix,  // period is zero
    "2014/0" + value_suffix,       // period is zero
    "2014-" + value_suffix,
    "2014-2014" + value_suffix,
    "2014-2012" + value_suffix,
    "26:00-27:00" + value_suffix,
    "23:00-55:00" + value_suffix,
    "23:59-48:01" + value_suffix,
    "25am-26pm" + value_suffix,
    "24am-26pm" + value_suffix,
    "23am-49pm" + value_suffix,
    "10:am - 8:pm" + value_suffix,
    "25pm-26am" + value_suffix,
    "Tu 23:59-48:00+" + value_suffix, // Does not make much sense. Should be written in another way.
    "12:00" + value_suffix,
    "„testing„" + value_suffix,   // Garbage, no valid quotes what so ever.
    "‚testing‚" + value_suffix,   // Garbage, no valid quotes what so ever.
    "»testing«" + value_suffix,   // Garbage, no valid quotes what so ever.
    "」testing「" + value_suffix, // Garbage, no valid quotes what so ever.
    "』testing『" + value_suffix, // Garbage, no valid quotes what so ever.
    "』testing「" + value_suffix, // Garbage, no valid quotes what so ever.
    "』testing«" + value_suffix,  // Garbage, no valid quotes what so ever.
    "』testing\"" + value_suffix,  // Garbage, no valid quotes what so ever. There is a second comment in value_suffix so they get combined.
    "\"testing«" + value_suffix,   // Garbage, no valid quotes what so ever.
    " || open" + value_suffix,
    "|| open" + value_suffix,
    "PH, Aug-Sep 00:00-24:00" + value_suffix,
    "We off, Mo,Tu,Th-Su,PH, Jun-Aug We 11:00-14:00,17:00+" + value_suffix,
    "We, Aug Mo" + value_suffix,
    "2014, Aug Mo" + value_suffix,
    "week 5, Aug Mo" + value_suffix,
    "Jun 2-5, week 5 00:00-24:00" + value_suffix,
    "Jan 0" + value_suffix,
    "Jan 32" + value_suffix,
    "Feb 30" + value_suffix,
    "Mar 32" + value_suffix,
    "Apr 31" + value_suffix,
    "Mai 32" + value_suffix,
    "Jun 31" + value_suffix,
    "Jul 32" + value_suffix,
    "Aug 32" + value_suffix,
    "Sep 31" + value_suffix,
    "Oct 32" + value_suffix,
    "Nov 31" + value_suffix,
    "Dec 32" + value_suffix,
  });

  TestShouldFail("Missing information (e.g. country or holidays not known to opening_hours.js)", {
    "PH", // country is not specified
    "SH", // country is not specified
  });

  TestShouldFail("opening_hours.js is in the wrong mode.", {
    "Mo sunrise,sunset", // only in mode 1 or 2, default is 0
    "Mo sunrise-(sunrise+01:00)/60", // only in mode 1 or 2, default is 0
  });

  // }}}

  std::cout << "Total JS tests: " << countTests << std::endl;
}
