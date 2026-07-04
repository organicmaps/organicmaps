// opening_hours_test.cpp – Tests for the C++23 opening hours parser
// Compile: clang++ -std=c++23 -Wall -Wextra -o opening_hours_test opening_hours_test.cpp
//
// Three categories of tests:
//   1. Input normalization (Pass 1)
//   2. Parse + to_string roundtrip (Pass 2)
//   3. Dedup (Pass 3)

#include "3party/opening_hours/oh/parser.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

// ---- Minimal test framework ----
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST(name)                                                         \
    static void test_##name();                                             \
    struct TestReg_##name {                                                 \
        TestReg_##name() {                                                  \
            test_registry().push_back({#name, test_##name});               \
        }                                                                   \
    };                                                                      \
    static TestReg_##name test_reg_instance_##name;                        \
    static void test_##name()

struct TestEntry { const char* name; void (*func)(); };
static std::vector<TestEntry>& test_registry() {
    static std::vector<TestEntry> reg;
    return reg;
}

#define ASSERT_EQ(a, b)                                                     \
    do {                                                                    \
        auto&& _a = (a);                                                    \
        auto&& _b = (b);                                                    \
        if (_a != _b) {                                                     \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__          \
                      << "\n    Expected: " << _b                           \
                      << "\n    Got:      " << _a << "\n";                  \
            throw std::runtime_error("assertion failed");                   \
        }                                                                   \
    } while (0)

#define ASSERT_TRUE(cond)                                                   \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__          \
                      << "\n    Condition false: " #cond "\n";              \
            throw std::runtime_error("assertion failed");                   \
        }                                                                   \
    } while (0)

// ============================================================================
// Pass 1 – Input normalization tests
// ============================================================================

using opening_hours::normalize_input;

// --- AM/PM conversion ---
TEST(ampm_basic)           { ASSERT_EQ(normalize_input("8:00 AM-5:00 PM"), "08:00-17:00"); }
TEST(ampm_noon_midnight)   { ASSERT_EQ(normalize_input("12:00 PM-12:00 AM"), "12:00-00:00"); }
TEST(ampm_lowercase)       { ASSERT_EQ(normalize_input("9:30 am-6:30 pm"), "09:30-18:30"); }
TEST(ampm_no_space)        { ASSERT_EQ(normalize_input("8:00AM-5:00PM"), "08:00-17:00"); }
TEST(ampm_dot_separator)   { ASSERT_EQ(normalize_input("8.00 AM-5.00 PM"), "08:00-17:00"); }
TEST(ampm_12am)            { ASSERT_EQ(normalize_input("12:00 AM-12:00 PM"), "00:00-12:00"); }
TEST(ampm_12pm)            { ASSERT_EQ(normalize_input("12:30 PM"), "12:30"); }

TEST(ampm_not_amsterdam) {
    auto result = normalize_input("10:00 AMsterdam");
    ASSERT_TRUE(result.find("AMsterdam") != std::string::npos
             || result.find("amsterdam") != std::string::npos);
}

// --- French h-time conversion ---
TEST(h_time_with_minutes)  { ASSERT_EQ(normalize_input("8h30-17h30"), "08:30-17:30"); }
TEST(h_time_without_minutes) { ASSERT_EQ(normalize_input("8h-17h"), "08:00-17:00"); }
TEST(h_time_uppercase)     { ASSERT_EQ(normalize_input("8H30-17H30"), "08:30-17:30"); }
TEST(h_time_24)            { ASSERT_EQ(normalize_input("24h"), "24:00"); }
TEST(h_time_zero)          { ASSERT_EQ(normalize_input("0h-24h"), "00:00-24:00"); }

TEST(h_time_not_word) {
    auto result = normalize_input("PH 10:00-18:00");
    ASSERT_EQ(result, "PH 10:00-18:00");
}

// --- Dot-format time conversion ---
TEST(dot_time_basic)       { ASSERT_EQ(normalize_input("Mo 8.00-17.00"), "Mo 8:00-17:00"); }
TEST(dot_time_boundary_24) { ASSERT_EQ(normalize_input("0.00-24.00"), "0:00-24:00"); }

TEST(dot_time_non_time) {
    auto result = normalize_input("Mo 1.2.3.4");
    ASSERT_TRUE(result.find('.') != std::string::npos);
}

// --- Unicode dash normalization ---
TEST(en_dash) { ASSERT_EQ(normalize_input("Mo\xe2\x80\x93""Fr 10:00\xe2\x80\x93""18:00"), "Mo-Fr 10:00-18:00"); }
TEST(em_dash) { ASSERT_EQ(normalize_input("Mo\xe2\x80\x94""Fr 10:00\xe2\x80\x94""18:00"), "Mo-Fr 10:00-18:00"); }

// --- Full-width colon ---
TEST(fullwidth_colon) { ASSERT_EQ(normalize_input("Mo 10\xef\xbc\x9a""00-18\xef\xbc\x9a""00"), "Mo 10:00-18:00"); }

// --- Space collapsing ---
TEST(collapse_spaces) { ASSERT_EQ(normalize_input("Mo   10:00  -  18:00"), "Mo 10:00-18:00"); }

// --- Weekday keyword normalization ---
TEST(full_english_weekdays)  { ASSERT_EQ(normalize_input("Monday-Friday 10:00-18:00"), "Mo-Fr 10:00-18:00"); }
TEST(three_letter_weekdays)  { ASSERT_EQ(normalize_input("Mon-Fri 10:00-18:00"), "Mo-Fr 10:00-18:00"); }
TEST(german_full_weekdays)   { ASSERT_EQ(normalize_input("Montag-Freitag 10:00-18:00"), "Mo-Fr 10:00-18:00"); }
TEST(german_two_letter)      { ASSERT_EQ(normalize_input("Di-Do 10:00-18:00"), "Tu-Th 10:00-18:00"); }
TEST(case_insensitive_wd1)   { ASSERT_EQ(normalize_input("mo-fr 10:00-18:00"), "Mo-Fr 10:00-18:00"); }
TEST(case_insensitive_wd2)   { ASSERT_EQ(normalize_input("MO-FR 10:00-18:00"), "Mo-Fr 10:00-18:00"); }
TEST(weekday_trailing_dot)   { ASSERT_EQ(normalize_input("Mo. 10:00-18:00"), "Mo 10:00-18:00"); }
TEST(weekday_spaced_dash)    { ASSERT_EQ(normalize_input("Mo - Fr 10:00-18:00"), "Mo-Fr 10:00-18:00"); }
TEST(weekday_digit_space)    { ASSERT_EQ(normalize_input("Sa09:00-18:00"), "Sa 09:00-18:00"); }

// --- Holiday normalization ---
TEST(holidays_case)   { ASSERT_EQ(normalize_input("ph off"), "PH off"); }
TEST(holidays_sh)     { ASSERT_EQ(normalize_input("sh off"), "SH off"); }

// --- Modifier keywords ---
TEST(modifier_open)    { ASSERT_EQ(normalize_input("Mo OPEN"), "Mo open"); }
TEST(modifier_closed)  { ASSERT_EQ(normalize_input("Mo CLOSED"), "Mo closed"); }
TEST(modifier_unknown) { ASSERT_EQ(normalize_input("Mo UNKNOWN"), "Mo unknown"); }
TEST(modifier_off)     { ASSERT_EQ(normalize_input("Mo OFF"), "Mo off"); }

// --- Empty comment stripping ---
TEST(strip_empty_comment1) { ASSERT_EQ(normalize_input("\"\" Mo 10:00-18:00"), "Mo 10:00-18:00"); }
TEST(strip_empty_comment2) { ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"\""), "Mo 10:00-18:00"); }

// --- Quoted regions preserved ---
TEST(quoted_preserved) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"lunch break\""),
              "Mo 10:00-18:00 \"lunch break\"");
}

TEST(quoted_time_like_content_preserved) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"8:00 AM, 17h30, 18.00\""),
              "Mo 10:00-18:00 \"8:00 AM, 17h30, 18.00\"");
}

TEST(quoted_date_list_preserved) {
    ASSERT_EQ(normalize_input("\"Dec 24,31\" Mo"), "\"Dec 24,31\": Mo");
}

TEST(quoted_keywords_not_normalized) {
    ASSERT_EQ(normalize_input("\"Monday to Friday\" Mo-Fr"),
              "\"Monday to Friday\": Mo-Fr");
}

// --- Pipe normalization ---
TEST(single_pipe) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00 | Tu 10:00-18:00"),
              "Mo 10:00-18:00 ; Tu 10:00-18:00");
}
TEST(double_pipe_preserved) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00||Tu 10:00-18:00"),
              "Mo 10:00-18:00|| Tu 10:00-18:00");
}
TEST(double_pipe_space) { ASSERT_EQ(normalize_input("Mo||Tu"), "Mo|| Tu"); }

// --- Colon after weekday → space ---
TEST(colon_after_wd1) { ASSERT_EQ(normalize_input("Mo: 10:00-18:00"), "Mo 10:00-18:00"); }
TEST(colon_after_wd2) { ASSERT_EQ(normalize_input("Mo:10:00-18:00"), "Mo 10:00-18:00"); }

// --- Trailing punctuation ---
TEST(trim_trailing_semi)   { ASSERT_EQ(normalize_input("Mo 10:00-18:00;"), "Mo 10:00-18:00"); }
TEST(trim_trailing_period) { ASSERT_EQ(normalize_input("Mo 10:00-18:00."), "Mo 10:00-18:00"); }
TEST(trim_multiple_trail)  { ASSERT_EQ(normalize_input("Mo 10:00-18:00;;"), "Mo 10:00-18:00"); }

// --- Whitespace ---
TEST(trim_whitespace) { ASSERT_EQ(normalize_input("  Mo 10:00-18:00  "), "Mo 10:00-18:00"); }

// --- Double semicolons ---
TEST(collapse_double_semi) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00;; Tu 10:00-18:00"),
              "Mo 10:00-18:00; Tu 10:00-18:00");
}
TEST(collapse_spaced_double_semi) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00; ; Tu 10:00-18:00"),
              "Mo 10:00-18:00; Tu 10:00-18:00");
}

// --- Comment colon insertion ---
TEST(comment_colon_wd) {
    ASSERT_EQ(normalize_input("\"hours\" Mo 10:00-18:00"), "\"hours\": Mo 10:00-18:00");
}
TEST(comment_colon_time) {
    ASSERT_EQ(normalize_input("\"hours\" 10:00-18:00"), "\"hours\": 10:00-18:00");
}
TEST(comment_colon_holiday) {
    ASSERT_EQ(normalize_input("\"hours\" PH off"), "\"hours\": PH off");
}
TEST(comment_colon_24_7) {
    ASSERT_EQ(normalize_input("\"hours\" 24/7"), "\"hours\": 24/7");
}
TEST(comment_colon_already) {
    ASSERT_EQ(normalize_input("\"hours\": Mo 10:00-18:00"), "\"hours\": Mo 10:00-18:00");
}

// --- Date comma expansion ---
TEST(expand_date_comma)    { ASSERT_EQ(normalize_input("Dec 24,31"), "Dec 24,Dec 31"); }
TEST(expand_date_multiple) { ASSERT_EQ(normalize_input("Dec 24,25,31"), "Dec 24,Dec 25,Dec 31"); }
TEST(no_expand_no_comma)   { ASSERT_EQ(normalize_input("Dec 24"), "Dec 24"); }

// --- Semicolon insertion between time and weekday ---
TEST(semi_insert_time_wd) {
    ASSERT_EQ(normalize_input("10:00-18:00 Mo"), "10:00-18:00; Mo");
}
TEST(semi_insert_time_ph) {
    ASSERT_EQ(normalize_input("10:00-18:00 PH off"), "10:00-18:00; PH off");
}

// --- Comma-keyword space ---
TEST(comma_keyword_space) {
    ASSERT_EQ(normalize_input("10:00-12:00,Mo 14:00-16:00"),
              "10:00-12:00, Mo 14:00-16:00");
}
TEST(comma_no_space_weekdays) {
    ASSERT_EQ(normalize_input("Mo,Tu,We 10:00-18:00"), "Mo,Tu,We 10:00-18:00");
}

// --- Mixed scenarios ---
TEST(complex_mixed) {
    ASSERT_EQ(normalize_input("monday - friday  8.00 AM - 5.00 PM"),
              "Mo-Fr 08:00-17:00");
}

TEST(empty_input)     { ASSERT_EQ(normalize_input(""), ""); }
TEST(only_whitespace) { ASSERT_EQ(normalize_input("   "), ""); }
TEST(already_canon)   { ASSERT_EQ(normalize_input("Mo-Fr 10:00-18:00"), "Mo-Fr 10:00-18:00"); }
TEST(twenty_four_seven) { ASSERT_EQ(normalize_input("24/7"), "24/7"); }

TEST(unicode_preserved) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"Büro\""),
              "Mo 10:00-18:00 \"Büro\"");
}

TEST(multiple_pipes) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00|Tu 10:00-18:00|We 10:00-18:00"),
              "Mo 10:00-18:00;Tu 10:00-18:00;We 10:00-18:00");
}

TEST(sunrise_sunset_kw) {
    ASSERT_EQ(normalize_input("sunrise-sunset"), "sunrise-sunset");
    ASSERT_EQ(normalize_input("SUNRISE-SUNSET"), "sunrise-sunset");
}
TEST(dawn_dusk_kw) {
    ASSERT_EQ(normalize_input("dawn-dusk"), "dawn-dusk");
    ASSERT_EQ(normalize_input("DAWN-DUSK"), "dawn-dusk");
}

TEST(month_kw_case) {
    ASSERT_EQ(normalize_input("JAN-MAR"), "Jan-Mar");
    ASSERT_EQ(normalize_input("jan-mar"), "Jan-Mar");
}

TEST(weekday_range_all) {
    ASSERT_EQ(normalize_input("Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday"),
              "Mo,Tu,We,Th,Fr,Sa,Su");
}
TEST(german_weekday_range) {
    ASSERT_EQ(normalize_input("Montag-Sonntag 10:00-18:00"), "Mo-Su 10:00-18:00");
}

TEST(multiple_empty_comments) {
    ASSERT_EQ(normalize_input("\"\" \"\" Mo 10:00-18:00"), "Mo 10:00-18:00");
}
TEST(unmatched_quote) {
    ASSERT_EQ(normalize_input("Mo 10:00-18:00 \"open"), "Mo 10:00-18:00 \"open");
}

TEST(comment_with_sh) {
    ASSERT_EQ(normalize_input("\"hours\" SH off"), "\"hours\": SH off");
}

// ============================================================================
// Pass 2 – Parse tests
// ============================================================================

using opening_hours::parse;
using opening_hours::parse_normalized;

TEST(parse_24_7) {
    auto r = parse("24/7");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "24/7");
}

TEST(parse_simple_weekday_time) {
    auto r = parse("Mo-Fr 10:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo-Fr 10:00-18:00");
}

TEST(parse_closed) {
    auto r = parse("Mo-Fr 10:00-18:00 closed");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo-Fr 10:00-18:00 closed");
}

TEST(parse_multiple_rules) {
    auto r = parse("Mo-Fr 08:00-18:00 ; Sa 10:00-14:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo-Fr 08:00-18:00; Sa 10:00-14:00");
}

TEST(parse_holidays) {
    auto r = parse("PH off");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "PH closed");
}

TEST(parse_single_weekday) {
    auto r = parse("Mo 08:00-12:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo 08:00-12:00");
}

TEST(parse_open_end) {
    auto r = parse("Mo 08:00+");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo 08:00+");
}

TEST(parse_time_span_with_repeat) {
    auto r = parse("Mo 08:00-18:00/02:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo 08:00-18:00/02:00");
}

TEST(parse_dawn_dusk) {
    auto r = parse("dawn-dusk");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "dawn-dusk");
}

TEST(parse_sunrise_sunset) {
    auto r = parse("sunrise-sunset");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "sunrise-sunset");
}

TEST(parse_variable_time_offset) {
    auto r = parse("(sunrise+01:00)-(sunset-01:00)");
    ASSERT_TRUE(r.has_value());
    // Check it parsed successfully
    ASSERT_TRUE(r->rules.size() == 1);
}

TEST(parse_year_easter_range_with_offsets) {
    auto r = parse("2012 easter -2 days-2012 easter +2 days: open \"Around easter\"; PH off");
    ASSERT_TRUE(r.has_value());
}

TEST(parse_easter_case_insensitive) {
    auto r = parse("Easter-Oct 31 Mo-Su 10:00-17:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "easter-Oct 31 Mo-Su 10:00-17:00");
}

TEST(parse_month_range) {
    auto r = parse("Nov-Mar Mo-Fr 10:00-16:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Nov-Mar Mo-Fr 10:00-16:00");
}

TEST(parse_month_with_weekday_list) {
    auto r = parse("Apr Mo,Th 11:00-17:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Apr Mo,Th 11:00-17:00");
}

TEST(parse_month_weekday_in_month_requires_nth) {
    auto r = parse("Feb Mo[1] +2 days");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Feb Mo[1] +2 days");
}

TEST(parse_month_date_range) {
    auto r = parse("Dec 24-Jan 2");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 1);
}

TEST(parse_week_selector) {
    auto r = parse("week02-02/7");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 1);
}

TEST(parse_fallback_rule) {
    auto r = parse("Mo-Fr 08:00-18:00 || 10:00-14:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 2);
    ASSERT_TRUE(r->rules[1].op == opening_hours::RuleOperator::Fallback);
}

TEST(parse_additional_rule) {
    // A spaced comma between two timespans is a time list, not an additive rule
    // (opening-hours-rs gh88): so an additive rule needs a following selector.
    auto r = parse("Mo-Fr 08:00-12:00, Sa 14:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 2);
    ASSERT_TRUE(r->rules[1].op == opening_hours::RuleOperator::Additional);
}

TEST(parse_time_list_with_space) {
    // gh88: comma in a time block wins over the additive separator, even spaced.
    auto r = parse("Mo-Fr 08:00-12:00, 14:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 1);
    ASSERT_TRUE(r->rules[0].time_selector.time.size() == 2);
}

TEST(reject_inverted_year_range) {
    ASSERT_TRUE(!parse("2030-2020").has_value());   // inverted -> rejected (as in Rust)
    ASSERT_TRUE(parse("2020-2030").has_value());     // ascending range is fine
    ASSERT_TRUE(parse("2020-2020").has_value());     // single value is not inverted
    ASSERT_TRUE(parse("2020+").has_value());         // open-ended is not inverted
}

TEST(reject_inverted_week_range) {
    ASSERT_TRUE(!parse("week10-05").has_value());
    ASSERT_TRUE(parse("week05-10").has_value());
    ASSERT_TRUE(parse("week02-02").has_value());
}

TEST(parse_comment) {
    auto r = parse("Mo 08:00-12:00 \"by appointment\"");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules[0].comments.size() == 1);
    ASSERT_EQ(r->rules[0].comments[0], "by appointment");
}

TEST(parse_comment_label) {
    auto r = parse("\"hours\": Mo 10:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 1);
}

TEST(parse_comment_label_24_7) {
    auto r = parse("\"Emergency\" 24/7; \"Office\" Mo-Th 07:30-17:30; Fr 07:30-17:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 3);
}

TEST(parse_year_range) {
    auto r = parse("2022 Mo-Fr 10:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules[0].day_selector.year.size() == 1);
    ASSERT_EQ(r->rules[0].day_selector.year[0].start, 2022);
}

TEST(parse_unknown_modifier) {
    auto r = parse("Mo unknown");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules[0].kind == opening_hours::RuleKind::Unknown);
}

TEST(parse_extended_time) {
    auto r = parse("Mo 20:00-02:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo 20:00-02:00");
}

TEST(parse_multiple_timespans) {
    auto r = parse("Mo 08:00-12:00,14:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo 08:00-12:00,14:00-18:00");
}

TEST(parse_weekday_nth) {
    auto r = parse("Fr[1] 08:00-12:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 1);
}

TEST(parse_school_holiday) {
    auto r = parse("SH 10:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 1);
}

TEST(parse_full_pipeline_normalization) {
    // Test that the full pipeline normalizes then parses
    auto r = parse("Monday-Friday 8.00 AM - 5.00 PM");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo-Fr 08:00-17:00");
}

TEST(parse_24_7_closed) {
    auto r = parse("24/7 closed");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "24/7 closed");
}

TEST(parse_complex_1) {
    auto r = parse("Mo-Fr 08:00-18:00 ; Sa 10:00-14:00 ; PH off");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 3);
}

TEST(parse_multiple_weekdays_comma) {
    auto r = parse("Mo,Tu,We,Th,Fr 10:00-18:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r->rules.size() == 1);
    ASSERT_TRUE(r->rules[0].day_selector.weekday.size() == 5);
}

TEST(parse_jun_range) {
    auto r = parse("Jun 24:00+");
    ASSERT_TRUE(r.has_value());
}

TEST(parse_sep_time) {
    auto r = parse("Sep 24:00-04:20");
    ASSERT_TRUE(r.has_value());
}

TEST(parse_dusk_to_dusk) {
    auto r = parse("dusk-dusk");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "dusk-dusk");
}

TEST(parse_dusk_48_open) {
    auto r = parse("dusk-48:00+");
    ASSERT_TRUE(r.has_value());
}

// ============================================================================
// Pass 3 – Dedup tests
// ============================================================================

TEST(dedup_removes_exact_duplicates) {
    // Parse two identical rules deliberately
    auto r = parse_normalized("Mo 10:00-18:00 ; Mo 10:00-18:00");
    ASSERT_TRUE(r.has_value());
    auto d = opening_hours::dedup(std::move(*r));
    ASSERT_EQ(d.rules.size(), 1u);
    ASSERT_EQ(d.to_string(), "Mo 10:00-18:00");
}

TEST(dedup_merges_adjacent_timespans) {
    auto r = parse_normalized("Mo 08:00-12:00,12:00-18:00");
    ASSERT_TRUE(r.has_value());
    auto d = opening_hours::dedup(std::move(*r));
    ASSERT_EQ(d.rules.size(), 1u);
    // The two adjacent spans should merge into one
    ASSERT_EQ(d.rules[0].time_selector.time.size(), 1u);
    ASSERT_EQ(d.to_string(), "Mo 08:00-18:00");
}

TEST(dedup_merges_overlapping_timespans) {
    auto r = parse_normalized("Mo 08:00-14:00,12:00-18:00");
    ASSERT_TRUE(r.has_value());
    auto d = opening_hours::dedup(std::move(*r));
    ASSERT_EQ(d.rules[0].time_selector.time.size(), 1u);
    ASSERT_EQ(d.to_string(), "Mo 08:00-18:00");
}

TEST(dedup_keeps_non_overlapping) {
    auto r = parse_normalized("Mo 08:00-12:00,14:00-18:00");
    ASSERT_TRUE(r.has_value());
    auto d = opening_hours::dedup(std::move(*r));
    ASSERT_EQ(d.rules[0].time_selector.time.size(), 2u);
}

TEST(dedup_removes_duplicate_comments) {
    auto r = parse_normalized("Mo 08:00-12:00 \"test\"");
    ASSERT_TRUE(r.has_value());
    // Manually add duplicate comment
    r->rules[0].comments.push_back("test");
    auto d = opening_hours::dedup(std::move(*r));
    ASSERT_EQ(d.rules[0].comments.size(), 1u);
}

TEST(dedup_different_rules_kept) {
    auto r = parse_normalized("Mo 10:00-18:00 ; Tu 10:00-18:00");
    ASSERT_TRUE(r.has_value());
    auto d = opening_hours::dedup(std::move(*r));
    ASSERT_EQ(d.rules.size(), 2u);
}

// ============================================================================
// Roundtrip tests: parse(to_string(parse(x))) == parse(x)
// ============================================================================

TEST(roundtrip_24_7) {
    auto r1 = parse("24/7");
    ASSERT_TRUE(r1.has_value());
    auto s = r1->to_string();
    auto r2 = parse(s);
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r1->to_string(), r2->to_string());
}

TEST(roundtrip_complex) {
    auto r1 = parse("Mo-Fr 08:00-18:00 ; Sa 10:00-14:00 ; PH off");
    ASSERT_TRUE(r1.has_value());
    auto s = r1->to_string();
    auto r2 = parse(s);
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r1->to_string(), r2->to_string());
}

TEST(roundtrip_month) {
    auto r1 = parse("Nov-Mar Mo-Fr 10:00-16:00");
    ASSERT_TRUE(r1.has_value());
    auto s = r1->to_string();
    auto r2 = parse(s);
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r1->to_string(), r2->to_string());
}

TEST(roundtrip_fallback) {
    auto r1 = parse("Mo-Fr 08:00-18:00 || 10:00-14:00");
    ASSERT_TRUE(r1.has_value());
    auto s = r1->to_string();
    auto r2 = parse(s);
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r1->to_string(), r2->to_string());
}

TEST(roundtrip_open_end) {
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

TEST(display_variable_time_offset) {
    // Offsets are parenthesized HH:MM, not raw signed minutes.
    auto r = parse("(sunrise-00:10)-(sunset+01:15)");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "(sunrise-00:10)-(sunset+01:15)");
}

TEST(display_year_single_value_with_step) {
    // '/' step is invalid on a single year: drop both the "-end" and the step.
    auto r = parse("2020-2020/7");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "2020");
}

TEST(display_week_single_value) {
    // A single week collapses to "weekNN".
    auto r = parse("week02-02/7");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "week02");
}

TEST(week_step_clamped_when_exceeding_span) {
    // A step larger than the range span is clamped to 1 (matches Rust
    // `WeekRange::new`), affecting both to_string and evaluation.
    auto r = parse("week04-24/71");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "week04-24");
    // A valid step (<= span) is preserved.
    auto r2 = parse("week01-53/2");
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r2->to_string(), "week01-53/2");
}

TEST(year_step_clamped_when_exceeding_span) {
    auto r = parse("2020-2025/71");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "2020-2025");
    auto r2 = parse("2010-2100/3");
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r2->to_string(), "2010-2100/3");
}

TEST(display_additional_rule_forces_weekday) {
    // An additional rule with no day selector must emit "Mo-Su", otherwise the
    // output re-parses as extra timespans of the previous rule.
    auto r = parse("Mo 10:00-12:00 , 13:00-14:00");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->to_string(), "Mo 10:00-12:00, Mo-Su 13:00-14:00");
}

// ============================================================================
// main
// ============================================================================

int main() {
    std::cout << "Running " << test_registry().size() << " tests...\n";

    for (auto& [name, func] : test_registry()) {
        ++g_tests_run;
        try {
            func();
            ++g_tests_passed;
            std::cout << "  PASS: " << name << "\n";
        } catch (const std::exception& e) {
            ++g_tests_failed;
            std::cerr << "  FAIL: " << name << " - " << e.what() << "\n";
        }
    }

    std::cout << "\n" << g_tests_passed << " / " << g_tests_run << " passed";
    if (g_tests_failed) std::cout << ", " << g_tests_failed << " FAILED";
    std::cout << "\n";

    return g_tests_failed ? 1 : 0;
}
