// Tests for the C++23 opening hours parser (oh/parser.hpp).
// The vectors are transcribed from the opening-hours-rs Rust suite; the bodies
// are kept intact, with the assertion macros mapped onto the OM framework.
//
// Three categories of tests:
//   1. Input normalization (Pass 1)
//   2. Parse + to_string roundtrip (Pass 2)
//   3. Dedup (Pass 3)

#include "opening_hours/oh/parser.hpp"

#include "testing/testing.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace oh_parser_tests
{
// Map the port suite's assertion macros onto the OM test framework.
#define ASSERT_EQ(a, b)   TEST_EQUAL(a, b, ())
#define ASSERT_TRUE(cond) TEST(cond, ())

// ============================================================================
// Pass 1 – Input normalization tests
// ============================================================================

using oh::normalize_input;

// --- AM/PM conversion ---
UNIT_TEST(ampm_basic)
{
  ASSERT_EQ(normalize_input("8:00 AM-5:00 PM"), "08:00-17:00");
}
UNIT_TEST(ampm_noon_midnight)
{
  ASSERT_EQ(normalize_input("12:00 PM-12:00 AM"), "12:00-00:00");
}
UNIT_TEST(ampm_lowercase)
{
  ASSERT_EQ(normalize_input("9:30 am-6:30 pm"), "09:30-18:30");
}
UNIT_TEST(ampm_no_space)
{
  ASSERT_EQ(normalize_input("8:00AM-5:00PM"), "08:00-17:00");
}
UNIT_TEST(ampm_dot_separator)
{
  ASSERT_EQ(normalize_input("8.00 AM-5.00 PM"), "08:00-17:00");
}
UNIT_TEST(ampm_12am)
{
  ASSERT_EQ(normalize_input("12:00 AM-12:00 PM"), "00:00-12:00");
}
UNIT_TEST(ampm_12pm)
{
  ASSERT_EQ(normalize_input("12:30 PM"), "12:30");
}

UNIT_TEST(ampm_not_amsterdam)
{
  auto result = normalize_input("10:00 AMsterdam");
  ASSERT_TRUE(result.find("AMsterdam") != std::string::npos || result.find("amsterdam") != std::string::npos);
}

// --- French h-time conversion ---
UNIT_TEST(h_time_with_minutes)
{
  ASSERT_EQ(normalize_input("8h30-17h30"), "08:30-17:30");
}
UNIT_TEST(h_time_without_minutes)
{
  ASSERT_EQ(normalize_input("8h-17h"), "08:00-17:00");
}
UNIT_TEST(h_time_uppercase)
{
  ASSERT_EQ(normalize_input("8H30-17H30"), "08:30-17:30");
}
UNIT_TEST(h_time_24)
{
  ASSERT_EQ(normalize_input("24h"), "24:00");
}
UNIT_TEST(h_time_zero)
{
  ASSERT_EQ(normalize_input("0h-24h"), "00:00-24:00");
}

UNIT_TEST(h_time_not_word)
{
  auto result = normalize_input("PH 10:00-18:00");
  ASSERT_EQ(result, "PH 10:00-18:00");
}

// --- Dot-format time conversion ---
UNIT_TEST(dot_time_basic)
{
  ASSERT_EQ(normalize_input("Mo 8.00-17.00"), "Mo 8:00-17:00");
}
UNIT_TEST(dot_time_boundary_24)
{
  ASSERT_EQ(normalize_input("0.00-24.00"), "0:00-24:00");
}

UNIT_TEST(dot_time_non_time)
{
  auto result = normalize_input("Mo 1.2.3.4");
  ASSERT_TRUE(result.find('.') != std::string::npos);
}

// --- Unicode dash normalization ---
UNIT_TEST(unicode_range_dashes)
{
  constexpr std::array<std::string_view, 10> dashes = {
      "\xE2\x80\x90", "\xE2\x80\x91", "\xE2\x80\x92", "\xE2\x80\x93", "\xE2\x80\x94",
      "\xE2\x80\x95", "\xE2\x88\x92", "\xEF\xB9\x98", "\xEF\xB9\xA3", "\xEF\xBC\x8D",
  };
  for (auto const dash : dashes)
  {
    ASSERT_EQ(normalize_input("Mo" + std::string(dash) + "Fr 10:00" + std::string(dash) + "18:00"),
              "Mo-Fr 10:00-18:00");
    ASSERT_EQ(normalize_input("\"Mo" + std::string(dash) + "Fr\" Mo-Fr"), "\"Mo" + std::string(dash) + "Fr\": Mo-Fr");
  }
}

// --- Full-width colon ---
UNIT_TEST(fullwidth_colon)
{
  ASSERT_EQ(normalize_input("Mo 10\xef\xbc\x9a"
                            "00-18\xef\xbc\x9a"
                            "00"),
            "Mo 10:00-18:00");
}

// --- Space collapsing ---
UNIT_TEST(collapse_spaces)
{
  ASSERT_EQ(normalize_input("Mo   10:00  -  18:00"), "Mo 10:00-18:00");
}

// --- Weekday keyword normalization ---
UNIT_TEST(full_english_weekdays)
{
  ASSERT_EQ(normalize_input("Monday-Friday 10:00-18:00"), "Mo-Fr 10:00-18:00");
}
UNIT_TEST(three_letter_weekdays)
{
  ASSERT_EQ(normalize_input("Mon-Fri 10:00-18:00"), "Mo-Fr 10:00-18:00");
}
UNIT_TEST(german_full_weekdays)
{
  ASSERT_EQ(normalize_input("Montag-Freitag 10:00-18:00"), "Mo-Fr 10:00-18:00");
}
UNIT_TEST(german_two_letter)
{
  ASSERT_EQ(normalize_input("Di-Do 10:00-18:00"), "Tu-Th 10:00-18:00");
}
UNIT_TEST(case_insensitive_wd1)
{
  ASSERT_EQ(normalize_input("mo-fr 10:00-18:00"), "Mo-Fr 10:00-18:00");
}
UNIT_TEST(case_insensitive_wd2)
{
  ASSERT_EQ(normalize_input("MO-FR 10:00-18:00"), "Mo-Fr 10:00-18:00");
}
UNIT_TEST(weekday_trailing_dot)
{
  ASSERT_EQ(normalize_input("Mo. 10:00-18:00"), "Mo 10:00-18:00");
}
UNIT_TEST(weekday_spaced_dash)
{
  ASSERT_EQ(normalize_input("Mo - Fr 10:00-18:00"), "Mo-Fr 10:00-18:00");
}
UNIT_TEST(weekday_digit_space)
{
  ASSERT_EQ(normalize_input("Sa09:00-18:00"), "Sa 09:00-18:00");
}

// --- Holiday normalization ---
UNIT_TEST(holidays_case)
{
  ASSERT_EQ(normalize_input("ph off"), "PH off");
}
UNIT_TEST(holidays_sh)
{
  ASSERT_EQ(normalize_input("sh off"), "SH off");
}

// --- Modifier keywords ---
UNIT_TEST(modifier_open)
{
  ASSERT_EQ(normalize_input("Mo OPEN"), "Mo open");
}
UNIT_TEST(modifier_closed)
{
  ASSERT_EQ(normalize_input("Mo CLOSED"), "Mo closed");
}
UNIT_TEST(modifier_unknown)
{
  ASSERT_EQ(normalize_input("Mo UNKNOWN"), "Mo unknown");
}
UNIT_TEST(modifier_off)
{
  ASSERT_EQ(normalize_input("Mo OFF"), "Mo off");
}

// --- Empty comment stripping ---
UNIT_TEST(strip_empty_comment1)
{
  ASSERT_EQ(normalize_input("\"\" Mo 10:00-18:00"), "Mo 10:00-18:00");
}
UNIT_TEST(strip_empty_comment2)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"\""), "Mo 10:00-18:00");
}

// --- Quoted regions preserved ---
UNIT_TEST(quoted_preserved)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"lunch break\""), "Mo 10:00-18:00 \"lunch break\"");
}

UNIT_TEST(quoted_time_like_content_preserved)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"8:00 AM, 17h30, 18.00\""), "Mo 10:00-18:00 \"8:00 AM, 17h30, 18.00\"");
}

UNIT_TEST(quoted_date_list_preserved)
{
  ASSERT_EQ(normalize_input("\"Dec 24,31\" Mo"), "\"Dec 24,31\": Mo");
}

UNIT_TEST(quoted_keywords_not_normalized)
{
  ASSERT_EQ(normalize_input("\"Monday to Friday\" Mo-Fr"), "\"Monday to Friday\": Mo-Fr");
}

// --- Pipe normalization ---
UNIT_TEST(single_pipe)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00 | Tu 10:00-18:00"), "Mo 10:00-18:00 ; Tu 10:00-18:00");
}
UNIT_TEST(double_pipe_preserved)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00||Tu 10:00-18:00"), "Mo 10:00-18:00|| Tu 10:00-18:00");
}
UNIT_TEST(double_pipe_space)
{
  ASSERT_EQ(normalize_input("Mo||Tu"), "Mo|| Tu");
}

// --- Colon after weekday → space ---
UNIT_TEST(colon_after_wd1)
{
  ASSERT_EQ(normalize_input("Mo: 10:00-18:00"), "Mo 10:00-18:00");
}
UNIT_TEST(colon_after_wd2)
{
  ASSERT_EQ(normalize_input("Mo:10:00-18:00"), "Mo 10:00-18:00");
}

// --- Trailing punctuation ---
UNIT_TEST(trim_trailing_semi)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00;"), "Mo 10:00-18:00");
}
UNIT_TEST(trim_trailing_period)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00."), "Mo 10:00-18:00");
}
UNIT_TEST(trim_multiple_trail)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00;;"), "Mo 10:00-18:00");
}

// --- Whitespace ---
UNIT_TEST(trim_whitespace)
{
  ASSERT_EQ(normalize_input("  Mo 10:00-18:00  "), "Mo 10:00-18:00");
}

// --- Double semicolons ---
UNIT_TEST(collapse_double_semi)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00;; Tu 10:00-18:00"), "Mo 10:00-18:00; Tu 10:00-18:00");
}
UNIT_TEST(collapse_spaced_double_semi)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00; ; Tu 10:00-18:00"), "Mo 10:00-18:00; Tu 10:00-18:00");
}

// --- Comment colon insertion ---
UNIT_TEST(comment_colon_wd)
{
  ASSERT_EQ(normalize_input("\"hours\" Mo 10:00-18:00"), "\"hours\": Mo 10:00-18:00");
}
UNIT_TEST(comment_colon_time)
{
  ASSERT_EQ(normalize_input("\"hours\" 10:00-18:00"), "\"hours\": 10:00-18:00");
}
UNIT_TEST(comment_colon_holiday)
{
  ASSERT_EQ(normalize_input("\"hours\" PH off"), "\"hours\": PH off");
}
UNIT_TEST(comment_colon_24_7)
{
  ASSERT_EQ(normalize_input("\"hours\" 24/7"), "\"hours\": 24/7");
}
UNIT_TEST(comment_colon_already)
{
  ASSERT_EQ(normalize_input("\"hours\": Mo 10:00-18:00"), "\"hours\": Mo 10:00-18:00");
}

// --- Date comma expansion ---
UNIT_TEST(expand_date_comma)
{
  ASSERT_EQ(normalize_input("Dec 24,31"), "Dec 24,Dec 31");
}
UNIT_TEST(expand_date_multiple)
{
  ASSERT_EQ(normalize_input("Dec 24,25,31"), "Dec 24,Dec 25,Dec 31");
}
UNIT_TEST(no_expand_no_comma)
{
  ASSERT_EQ(normalize_input("Dec 24"), "Dec 24");
}

// --- Semicolon insertion between time and weekday ---
UNIT_TEST(semi_insert_time_wd)
{
  ASSERT_EQ(normalize_input("10:00-18:00 Mo"), "10:00-18:00; Mo");
}
UNIT_TEST(semi_insert_time_ph)
{
  ASSERT_EQ(normalize_input("10:00-18:00 PH off"), "10:00-18:00; PH off");
}

// --- Comma-keyword space ---
UNIT_TEST(comma_keyword_space)
{
  ASSERT_EQ(normalize_input("10:00-12:00,Mo 14:00-16:00"), "10:00-12:00, Mo 14:00-16:00");
}
UNIT_TEST(comma_no_space_weekdays)
{
  ASSERT_EQ(normalize_input("Mo,Tu,We 10:00-18:00"), "Mo,Tu,We 10:00-18:00");
}

// --- Mixed scenarios ---
UNIT_TEST(complex_mixed)
{
  ASSERT_EQ(normalize_input("monday - friday  8.00 AM - 5.00 PM"), "Mo-Fr 08:00-17:00");
}

UNIT_TEST(empty_input)
{
  ASSERT_EQ(normalize_input(""), "");
}
UNIT_TEST(only_whitespace)
{
  ASSERT_EQ(normalize_input("   "), "");
}
UNIT_TEST(already_canon)
{
  ASSERT_EQ(normalize_input("Mo-Fr 10:00-18:00"), "Mo-Fr 10:00-18:00");
}
UNIT_TEST(twenty_four_seven)
{
  ASSERT_EQ(normalize_input("24/7"), "24/7");
}

UNIT_TEST(unicode_preserved)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"Büro\""), "Mo 10:00-18:00 \"Büro\"");
}

UNIT_TEST(multiple_pipes)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00|Tu 10:00-18:00|We 10:00-18:00"),
            "Mo 10:00-18:00;Tu 10:00-18:00;We 10:00-18:00");
}

UNIT_TEST(sunrise_sunset_kw)
{
  ASSERT_EQ(normalize_input("sunrise-sunset"), "sunrise-sunset");
  ASSERT_EQ(normalize_input("SUNRISE-SUNSET"), "sunrise-sunset");
}
UNIT_TEST(dawn_dusk_kw)
{
  ASSERT_EQ(normalize_input("dawn-dusk"), "dawn-dusk");
  ASSERT_EQ(normalize_input("DAWN-DUSK"), "dawn-dusk");
}

UNIT_TEST(month_kw_case)
{
  ASSERT_EQ(normalize_input("JAN-MAR"), "Jan-Mar");
  ASSERT_EQ(normalize_input("jan-mar"), "Jan-Mar");
}

UNIT_TEST(weekday_range_all)
{
  ASSERT_EQ(normalize_input("Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday"), "Mo,Tu,We,Th,Fr,Sa,Su");
}
UNIT_TEST(german_weekday_range)
{
  ASSERT_EQ(normalize_input("Montag-Sonntag 10:00-18:00"), "Mo-Su 10:00-18:00");
}

UNIT_TEST(multiple_empty_comments)
{
  ASSERT_EQ(normalize_input("\"\" \"\" Mo 10:00-18:00"), "Mo 10:00-18:00");
}
UNIT_TEST(unmatched_quote)
{
  ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"open"), "Mo 10:00-18:00 \"open");
}

UNIT_TEST(comment_with_sh)
{
  ASSERT_EQ(normalize_input("\"hours\" SH off"), "\"hours\": SH off");
}

// ============================================================================
// Pass 2 – Parse tests
// ============================================================================

using oh::parse;
using oh::parse_normalized;

UNIT_TEST(parse_24_7)
{
  auto r = parse("24/7");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "24/7");
}

UNIT_TEST(parse_simple_weekday_time)
{
  auto r = parse("Mo-Fr 10:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo-Fr 10:00-18:00");
}

UNIT_TEST(parse_closed)
{
  auto r = parse("Mo-Fr 10:00-18:00 closed");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo-Fr 10:00-18:00 closed");
}

UNIT_TEST(parse_multiple_rules)
{
  auto r = parse("Mo-Fr 08:00-18:00 ; Sa 10:00-14:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo-Fr 08:00-18:00; Sa 10:00-14:00");
}

UNIT_TEST(parse_holidays)
{
  auto r = parse("PH off");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "PH closed");
}

UNIT_TEST(parse_single_weekday)
{
  auto r = parse("Mo 08:00-12:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo 08:00-12:00");
}

UNIT_TEST(parse_open_end)
{
  auto r = parse("Mo 08:00+");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo 08:00+");
}

UNIT_TEST(parse_time_span_with_repeat)
{
  auto r = parse("Mo 08:00-18:00/02:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo 08:00-18:00/02:00");
}

UNIT_TEST(parse_dawn_dusk)
{
  auto r = parse("dawn-dusk");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "dawn-dusk");
}

UNIT_TEST(parse_sunrise_sunset)
{
  auto r = parse("sunrise-sunset");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "sunrise-sunset");
}

UNIT_TEST(parse_variable_time_offset)
{
  auto r = parse("(sunrise+01:00)-(sunset-01:00)");
  ASSERT_TRUE(r.has_value());
  // Check it parsed successfully
  ASSERT_TRUE(r->rules.size() == 1);
}

UNIT_TEST(parse_year_easter_range_with_offsets)
{
  auto r = parse("2012 easter -2 days-2012 easter +2 days: open \"Around easter\"; PH off");
  ASSERT_TRUE(r.has_value());
}

UNIT_TEST(parse_easter_case_insensitive)
{
  auto r = parse("Easter-Oct 31 Mo-Su 10:00-17:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "easter-Oct 31 Mo-Su 10:00-17:00");
}

UNIT_TEST(parse_month_range)
{
  auto r = parse("Nov-Mar Mo-Fr 10:00-16:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Nov-Mar Mo-Fr 10:00-16:00");
}

UNIT_TEST(parse_month_with_weekday_list)
{
  auto r = parse("Apr Mo,Th 11:00-17:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Apr Mo,Th 11:00-17:00");
}

UNIT_TEST(parse_month_weekday_in_month_requires_nth)
{
  auto r = parse("Feb Mo[1] +2 days");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Feb Mo[1] +2 days");
}

UNIT_TEST(parse_month_date_range)
{
  auto r = parse("Dec 24-Jan 2");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 1);
}

UNIT_TEST(parse_week_selector)
{
  auto r = parse("week02-02/7");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 1);
}

UNIT_TEST(parse_fallback_rule)
{
  auto r = parse("Mo-Fr 08:00-18:00 || 10:00-14:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 2);
  ASSERT_TRUE(r->rules[1].op == oh::RuleOperator::Fallback);
}

UNIT_TEST(parse_additional_rule)
{
  // A spaced comma between two timespans is a time list, not an additive rule
  // (opening-hours-rs gh88): so an additive rule needs a following selector.
  auto r = parse("Mo-Fr 08:00-12:00, Sa 14:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 2);
  ASSERT_TRUE(r->rules[1].op == oh::RuleOperator::Additional);
}

UNIT_TEST(parse_time_list_with_space)
{
  // gh88: comma in a time block wins over the additive separator, even spaced.
  auto r = parse("Mo-Fr 08:00-12:00, 14:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 1);
  ASSERT_TRUE(r->rules[0].time_selector.time.size() == 2);
}

UNIT_TEST(reject_inverted_year_range)
{
  ASSERT_TRUE(!parse("2030-2020").has_value());  // inverted -> rejected (as in Rust)
  ASSERT_TRUE(parse("2020-2030").has_value());   // ascending range is fine
  ASSERT_TRUE(parse("2020-2020").has_value());   // single value is not inverted
  ASSERT_TRUE(parse("2020+").has_value());       // open-ended is not inverted
}

UNIT_TEST(reject_inverted_week_range)
{
  ASSERT_TRUE(!parse("week10-05").has_value());
  ASSERT_TRUE(parse("week05-10").has_value());
  ASSERT_TRUE(parse("week02-02").has_value());
}

UNIT_TEST(parse_comment)
{
  auto r = parse("Mo 08:00-12:00 \"by appointment\"");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules[0].comments.size() == 1);
  ASSERT_EQ(r->rules[0].comments[0], "by appointment");
}

UNIT_TEST(parse_comment_label)
{
  auto r = parse("\"hours\": Mo 10:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 1);
}

UNIT_TEST(parse_comment_label_24_7)
{
  auto r = parse("\"Emergency\" 24/7; \"Office\" Mo-Th 07:30-17:30; Fr 07:30-17:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 3);
}

UNIT_TEST(parse_year_range)
{
  auto r = parse("2022 Mo-Fr 10:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules[0].day_selector.year.size() == 1);
  ASSERT_EQ(r->rules[0].day_selector.year[0].start, 2022);
}

UNIT_TEST(parse_unknown_modifier)
{
  auto r = parse("Mo unknown");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules[0].kind == oh::RuleKind::Unknown);
}

UNIT_TEST(parse_extended_time)
{
  auto r = parse("Mo 20:00-02:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo 20:00-02:00");
}

UNIT_TEST(parse_multiple_timespans)
{
  auto r = parse("Mo 08:00-12:00,14:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo 08:00-12:00,14:00-18:00");
}

UNIT_TEST(parse_weekday_nth)
{
  auto r = parse("Fr[1] 08:00-12:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 1);
}

UNIT_TEST(parse_school_holiday)
{
  auto r = parse("SH 10:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 1);
}

UNIT_TEST(parse_full_pipeline_normalization)
{
  // Test that the full pipeline normalizes then parses
  auto r = parse("Monday-Friday 8.00 AM - 5.00 PM");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo-Fr 08:00-17:00");
}

UNIT_TEST(parse_24_7_closed)
{
  auto r = parse("24/7 closed");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "24/7 closed");
}

UNIT_TEST(parse_complex_1)
{
  auto r = parse("Mo-Fr 08:00-18:00 ; Sa 10:00-14:00 ; PH off");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 3);
}

UNIT_TEST(parse_multiple_weekdays_comma)
{
  auto r = parse("Mo,Tu,We,Th,Fr 10:00-18:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->rules.size() == 1);
  ASSERT_TRUE(r->rules[0].day_selector.weekday.size() == 5);
}

UNIT_TEST(parse_jun_range)
{
  auto r = parse("Jun 24:00+");
  ASSERT_TRUE(r.has_value());
}

UNIT_TEST(parse_sep_time)
{
  auto r = parse("Sep 24:00-04:20");
  ASSERT_TRUE(r.has_value());
}

UNIT_TEST(parse_dusk_to_dusk)
{
  auto r = parse("dusk-dusk");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "dusk-dusk");
}

UNIT_TEST(parse_dusk_48_open)
{
  auto r = parse("dusk-48:00+");
  ASSERT_TRUE(r.has_value());
}

// ============================================================================
// Pass 3 – Dedup tests
// ============================================================================

UNIT_TEST(dedup_removes_exact_duplicates)
{
  // Parse two identical rules deliberately
  auto r = parse_normalized("Mo 10:00-18:00 ; Mo 10:00-18:00");
  ASSERT_TRUE(r.has_value());
  auto d = oh::dedup(std::move(*r));
  ASSERT_EQ(d.rules.size(), 1u);
  ASSERT_EQ(d.to_string(), "Mo 10:00-18:00");
}

UNIT_TEST(dedup_merges_adjacent_timespans)
{
  auto r = parse_normalized("Mo 08:00-12:00,12:00-18:00");
  ASSERT_TRUE(r.has_value());
  auto d = oh::dedup(std::move(*r));
  ASSERT_EQ(d.rules.size(), 1u);
  // The two adjacent spans should merge into one
  ASSERT_EQ(d.rules[0].time_selector.time.size(), 1u);
  ASSERT_EQ(d.to_string(), "Mo 08:00-18:00");
}

UNIT_TEST(dedup_merges_overlapping_timespans)
{
  auto r = parse_normalized("Mo 08:00-14:00,12:00-18:00");
  ASSERT_TRUE(r.has_value());
  auto d = oh::dedup(std::move(*r));
  ASSERT_EQ(d.rules[0].time_selector.time.size(), 1u);
  ASSERT_EQ(d.to_string(), "Mo 08:00-18:00");
}

UNIT_TEST(dedup_keeps_non_overlapping)
{
  auto r = parse_normalized("Mo 08:00-12:00,14:00-18:00");
  ASSERT_TRUE(r.has_value());
  auto d = oh::dedup(std::move(*r));
  ASSERT_EQ(d.rules[0].time_selector.time.size(), 2u);
}

UNIT_TEST(dedup_keeps_equal_ended_wrapping_span)
{
  auto r = parse_normalized("Mo 10:00-17:30,12:30-12:30");
  ASSERT_TRUE(r.has_value());
  auto d = oh::dedup(std::move(*r));
  ASSERT_EQ(d.rules[0].time_selector.time.size(), 2u);
}

UNIT_TEST(dedup_removes_duplicate_comments)
{
  auto r = parse_normalized("Mo 08:00-12:00 \"test\"");
  ASSERT_TRUE(r.has_value());
  // Manually add duplicate comment
  r->rules[0].comments.push_back("test");
  auto d = oh::dedup(std::move(*r));
  ASSERT_EQ(d.rules[0].comments.size(), 1u);
}

UNIT_TEST(dedup_different_rules_kept)
{
  auto r = parse_normalized("Mo 10:00-18:00 ; Tu 10:00-18:00");
  ASSERT_TRUE(r.has_value());
  auto d = oh::dedup(std::move(*r));
  ASSERT_EQ(d.rules.size(), 2u);
}

// ============================================================================
// Roundtrip tests: parse(to_string(parse(x))) == parse(x)
// ============================================================================

UNIT_TEST(roundtrip_24_7)
{
  auto r1 = parse("24/7");
  ASSERT_TRUE(r1.has_value());
  auto s = r1->to_string();
  auto r2 = parse(s);
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r1->to_string(), r2->to_string());
}

UNIT_TEST(roundtrip_complex)
{
  auto r1 = parse("Mo-Fr 08:00-18:00 ; Sa 10:00-14:00 ; PH off");
  ASSERT_TRUE(r1.has_value());
  auto s = r1->to_string();
  auto r2 = parse(s);
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r1->to_string(), r2->to_string());
}

UNIT_TEST(roundtrip_month)
{
  auto r1 = parse("Nov-Mar Mo-Fr 10:00-16:00");
  ASSERT_TRUE(r1.has_value());
  auto s = r1->to_string();
  auto r2 = parse(s);
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r1->to_string(), r2->to_string());
}

UNIT_TEST(roundtrip_fallback)
{
  auto r1 = parse("Mo-Fr 08:00-18:00 || 10:00-14:00");
  ASSERT_TRUE(r1.has_value());
  auto s = r1->to_string();
  auto r2 = parse(s);
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r1->to_string(), r2->to_string());
}

UNIT_TEST(roundtrip_open_end)
{
  auto r1 = parse("Mo 08:00+");
  ASSERT_TRUE(r1.has_value());
  auto s = r1->to_string();
  auto r2 = parse(s);
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r1->to_string(), r2->to_string());
}

// ============================================================================
// Display alignment with the Rust reference implementation
// (regressions for behavior fixed on master after this port was written)
// ============================================================================

UNIT_TEST(display_variable_time_offset)
{
  // Offsets are parenthesized HH:MM, not raw signed minutes.
  auto r = parse("(sunrise-00:10)-(sunset+01:15)");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "(sunrise-00:10)-(sunset+01:15)");
}

UNIT_TEST(display_year_single_value_with_step)
{
  // '/' step is invalid on a single year: drop both the "-end" and the step.
  auto r = parse("2020-2020/7");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "2020");
}

UNIT_TEST(display_week_single_value)
{
  // A single week collapses to "weekNN".
  auto r = parse("week02-02/7");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "week02");
}

UNIT_TEST(week_step_clamped_when_exceeding_span)
{
  // A step larger than the range span is clamped to 1 (matches Rust
  // `WeekRange::new`), affecting both to_string and evaluation.
  auto r = parse("week04-24/71");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "week04-24");
  // A valid step (<= span) is preserved.
  auto r2 = parse("week01-53/2");
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r2->to_string(), "week01-53/2");
  // A multiple of 256 must clamp too, not truncate to a zero step
  // (division by zero in evaluation).
  auto r3 = parse("week01-53/256");
  ASSERT_TRUE(r3.has_value());
  ASSERT_EQ(r3->to_string(), "week01-53");
}

UNIT_TEST(year_step_clamped_when_exceeding_span)
{
  auto r = parse("2020-2025/71");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "2020-2025");
  auto r2 = parse("2010-2100/3");
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r2->to_string(), "2010-2100/3");
  // Multiples of 65536 must clamp too, not truncate to a zero step
  // (division by zero in evaluation); 65538 would truncate to a silent 2.
  auto r3 = parse("2020-2030/65536");
  ASSERT_TRUE(r3.has_value());
  ASSERT_EQ(r3->to_string(), "2020-2030");
  auto r4 = parse("1900-9999/65538");
  ASSERT_TRUE(r4.has_value());
  ASSERT_EQ(r4->to_string(), "1900-9999");
}

UNIT_TEST(day_selector_24_7_means_all_day)
{
  // "Sa-Su 24/7" is invalid per the strict grammar but is the highest-weight
  // unhandled family in the real-world corpus; treat it as 00:00-24:00.
  auto r = parse("Sa-Su 24/7");
  ASSERT_TRUE(r.has_value());
  // A full-day span is the TimeSelector default, so it prints as bare days.
  ASSERT_EQ(r->to_string(), "Sa-Su");
  auto r2 = parse("week 20-37 24/7");
  ASSERT_TRUE(r2.has_value());
  ASSERT_EQ(r2->to_string(), "week20-37");
  ASSERT_TRUE(!parse("Sa-Su 24/78").has_value());
}

UNIT_TEST(inverted_nth_range_rejected)
{
  // Consistent with inverted year and week ranges; an empty nth set would
  // never match at evaluation yet round-trip as a plain weekday.
  ASSERT_TRUE(!parse("Mo[5-2] 10:00-12:00").has_value());
  ASSERT_TRUE(parse("Mo[2-5] 10:00-12:00").has_value());
}

UNIT_TEST(day_offset_bounded)
{
  // Day offsets must fit the AST representation without integer wrapping.
  ASSERT_TRUE(!parse("Sa +4294967296 days 10:00-12:00").has_value());
  ASSERT_TRUE(parse("Sa +2 days 10:00-12:00").has_value());
  ASSERT_TRUE(parse("PH -1 day 10:00-12:00").has_value());
}

UNIT_TEST(display_additional_rule_forces_weekday)
{
  // An additional rule with no day selector must emit "Mo-Su", otherwise the
  // output re-parses as extra timespans of the previous rule.
  auto r = parse("Mo 10:00-12:00 , 13:00-14:00");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->to_string(), "Mo 10:00-12:00, Mo-Su 13:00-14:00");
}

#undef ASSERT_EQ
#undef ASSERT_TRUE
}  // namespace oh_parser_tests
