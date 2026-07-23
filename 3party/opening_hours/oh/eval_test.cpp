// opening_hours_eval_test.cpp – Tests for the C++23 opening hours evaluator
// Compile:
//   clang++ -std=c++23 -Wall -Wextra -o opening_hours_eval_test opening_hours_eval_test.cpp
//   ./opening_hours_eval_test
//
// The golden vectors are transcribed from the Rust reference test suite
// (opening-hours/src/tests/eval_{state,schedule,next_change}.rs and
// localization.rs). The Schedule DSL ("10:00 open 12:00 closed …") mirrors
// tests/utils/parser_schedule.rs.

#include "3party/opening_hours/oh/eval.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace opening_hours;

// ---------------------------------------------------------------------------
// Minimal test framework
// ---------------------------------------------------------------------------
static int g_fail = 0;
static int g_pass = 0;
static const char* g_current = "";

#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        if (!(cond)) {                                                          \
            std::cerr << "  FAIL [" << g_current << "] " << msg << "\n";        \
            ++g_fail;                                                           \
        } else {                                                                \
            ++g_pass;                                                           \
        }                                                                       \
    } while (0)

// ---------------------------------------------------------------------------
// Parsing helpers for the test tables
// ---------------------------------------------------------------------------

// Parse "YYYY-MM-DD" (optionally leading '+') into a NaiveDate.
static NaiveDate pd(std::string_view s) {
    if (!s.empty() && s.front() == '+') s.remove_prefix(1);
    int y = 0, m = 0, d = 0;
    std::sscanf(std::string(s).c_str(), "%d-%d-%d", &y, &m, &d);
    return NaiveDate::ymd_unchecked(y, m, d);
}

// Parse "YYYY-MM-DD HH:MM" into a NaiveDateTime.
static NaiveDateTime pdt(std::string_view s) {
    if (!s.empty() && s.front() == '+') s.remove_prefix(1);
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    std::sscanf(std::string(s).c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi);
    return NaiveDateTime{NaiveDate::ymd_unchecked(y, mo, d), h * 60 + mi};
}

static OpeningHours<> OH(std::string_view s) {
    auto r = parse_opening_hours(s);
    if (!r) {
        std::cerr << "  PARSE ERROR for '" << s << "': " << r.error() << "\n";
        std::exit(2);
    }
    return *r;
}

// ---------------------------------------------------------------------------
// Schedule DSL: build a Schedule from "10:00 open 12:00 closed 13:00 …".
// Chains are separated by '|'; annotations are "kind[comment]".
// ---------------------------------------------------------------------------

static RuleKind parse_kind(std::string_view s) {
    if (s == "open" || s == "Open") return RuleKind::Open;
    if (s == "closed" || s == "Closed") return RuleKind::Closed;
    if (s == "unknown" || s == "Unknown") return RuleKind::Unknown;
    std::cerr << "bad kind: " << s << "\n";
    std::exit(2);
}

static ExtendedTime parse_et(std::string_view s) {
    int h = 0, m = 0;
    std::sscanf(std::string(s).c_str(), "%d:%d", &h, &m);
    return ExtendedTime{uint8_t(h), uint8_t(m)};
}

static Schedule schedule_from_dsl(std::string_view input) {
    Schedule schedule;
    // Split into chains on '|'.
    std::vector<std::string> chains;
    {
        std::string cur;
        for (char c : input) {
            if (c == '|') {
                chains.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        chains.push_back(cur);
    }

    for (const auto& chain : chains) {
        // Tokenize on whitespace, but keep "[...]" annotations (which may
        // contain spaces) attached to their rule token.
        std::vector<std::string> toks;
        std::string cur;
        bool in_bracket = false;
        for (char c : chain) {
            if (c == '[') in_bracket = true;
            if (c == ']') in_bracket = false;
            if (std::isspace((unsigned char)c) && !in_bracket) {
                if (!cur.empty()) {
                    toks.push_back(cur);
                    cur.clear();
                }
            } else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) toks.push_back(cur);
        if (toks.empty()) continue;

        ExtendedTime start = parse_et(toks[0]);
        for (size_t i = 1; i + 1 < toks.size(); i += 2) {
            std::string rule = toks[i];
            ExtendedTime end = parse_et(toks[i + 1]);

            std::string comment;
            auto lb = rule.find('[');
            if (lb != std::string::npos) {
                auto rb = rule.find(']', lb);
                comment = rule.substr(lb + 1, rb - lb - 1);
                rule = rule.substr(0, lb);
            }
            RuleKind kind = parse_kind(rule);

            schedule = schedule.addition(Schedule::from_ranges({ETRange{start, end}}, kind, comment));
            start = end;
        }
    }
    return schedule;
}

static std::string render(const Schedule& s) {
    std::string out;
    bool first = true;
    for (const auto& tr : s.into_ranges()) {
        if (!first) out += " | ";
        first = false;
        out += tr.start.to_string() + " " + rule_kind_str(tr.kind) + " " + tr.end.to_string();
        if (!tr.comment.empty()) out += "[" + tr.comment + "]";
    }
    return out;
}

// ---------------------------------------------------------------------------
// Comparators
// ---------------------------------------------------------------------------

template <class OH_T>
static void check_schedule(OH_T& oh, std::string_view date, std::string_view expected_dsl) {
    Schedule got = oh.schedule_at(pd(date));
    Schedule want = schedule_from_dsl(expected_dsl);
    CHECK(got == want, "schedule_at(" << oh.to_string() << " @ " << date << ")\n      got:  "
                                      << render(got) << "\n      want: " << render(want));
}

template <class OH_T>
static void check_schedule_empty(OH_T& oh, std::string_view date) {
    Schedule got = oh.schedule_at(pd(date));
    CHECK(got.is_empty(), "schedule_at(" << oh.to_string() << " @ " << date
                                         << ") expected empty, got: " << render(got));
}

static void check_state(std::string_view expr, std::string_view dt, RuleKind expected) {
    auto oh = OH(expr);
    auto st = oh.state(pdt(dt));
    CHECK(st.first == expected, "state(" << expr << " @ " << dt << ") = "
                                         << rule_kind_str(st.first) << " want "
                                         << rule_kind_str(expected));
}

static void check_next_change(std::string_view dt, std::string_view expr, std::string_view expected) {
    auto oh = OH(expr);
    auto nc = oh.next_change(pdt(dt));
    if (!nc) {
        CHECK(false, "next_change(" << expr << " @ " << dt << ") = <none>, want " << expected);
        return;
    }
    NaiveDateTime want = pdt(expected);
    CHECK(*nc == want, "next_change(" << expr << " @ " << dt << ") mismatch, want " << expected);
}

static void check_no_next_change(std::string_view dt, std::string_view expr) {
    auto oh = OH(expr);
    auto nc = oh.next_change(pdt(dt));
    CHECK(!nc.has_value(), "next_change(" << expr << " @ " << dt << ") should be <none>");
}

// ===========================================================================
// eval_state.rs
// ===========================================================================
static void test_eval_state() {
    g_current = "eval_state";
    // Date bounds
    check_state("24/7", "1789-07-14 12:00", RuleKind::Closed);
    check_state("24/7", "+10000-01-01 12:00", RuleKind::Closed);
    // Feb29
    check_state("Feb29", "2020-02-28 12:00", RuleKind::Closed);
    check_state("Feb29", "2020-02-29 12:00", RuleKind::Open);
    check_state("Feb29", "2020-03-01 12:00", RuleKind::Closed);
    check_state("Feb29", "2021-02-28 12:00", RuleKind::Closed);
    check_state("Feb29", "2021-03-01 12:00", RuleKind::Closed);
    check_state("Feb29-Mar15", "2020-02-28 12:00", RuleKind::Closed);
    check_state("Feb29-Mar15", "2020-02-29 12:00", RuleKind::Open);
    check_state("Feb29-Mar15", "2020-03-01 12:00", RuleKind::Open);
    check_state("Feb29-Mar15", "2020-03-16 12:00", RuleKind::Closed);
    check_state("Feb29-Mar15", "2021-02-28 12:00", RuleKind::Closed);
    check_state("Feb29-Mar15", "2021-03-01 12:00", RuleKind::Open);
    check_state("Feb29-Mar15", "2021-03-16 12:00", RuleKind::Closed);
    check_state("Feb15-Feb29", "2020-02-14 12:00", RuleKind::Closed);
    check_state("Feb15-Feb29", "2020-02-15 12:00", RuleKind::Open);
    check_state("Feb15-Feb29", "2020-02-29 12:00", RuleKind::Open);
    check_state("Feb15-Feb29", "2020-03-01 12:00", RuleKind::Closed);
    check_state("Feb15-Feb29", "2021-02-14 12:00", RuleKind::Closed);
    check_state("Feb15-Feb29", "2021-02-15 12:00", RuleKind::Open);
    check_state("Feb15-Feb29", "2021-02-28 12:00", RuleKind::Open);
    check_state("Feb15-Feb29", "2021-03-01 12:00", RuleKind::Closed);
    // Easter
    check_state("24/7 open ; easter off", "2024-03-30 12:00", RuleKind::Open);
    check_state("24/7 open ; easter off", "2024-03-31 12:00", RuleKind::Closed);
    check_state("24/7 open ; easter off", "2024-04-01 12:00", RuleKind::Open);
    check_state("Jan01-easter", "2023-12-31 12:00", RuleKind::Closed);
    check_state("Jan01-easter", "2024-01-01 12:00", RuleKind::Open);
    check_state("Jan01-easter", "2024-03-30 12:00", RuleKind::Open);
    check_state("Jan01-easter", "2024-03-31 12:00", RuleKind::Open);
    check_state("Jan01-easter", "2024-04-01 12:00", RuleKind::Closed);
    check_state("easter-Dec31", "2024-03-30 12:00", RuleKind::Closed);
    check_state("easter-Dec31", "2024-03-31 12:00", RuleKind::Open);
    check_state("easter-Dec31", "2024-12-31 12:00", RuleKind::Open);
    check_state("easter-Dec31", "2025-01-01 12:00", RuleKind::Closed);
    // Rule: additional / fallback
    check_state("Su closed || open", "2023-12-23 12:00", RuleKind::Open);
    check_state("closed; Mar || We", "2020-01-01 12:00", RuleKind::Open);
    // is_open/closed/unknown
    check_state("open", "2020-01-01 12:00", RuleKind::Open);
    check_state("closed", "2020-01-01 12:00", RuleKind::Closed);
    check_state("unknown", "2020-01-01 12:00", RuleKind::Unknown);
}

// ===========================================================================
// eval_schedule.rs  (NoLocation cases)
// ===========================================================================
static void test_eval_schedule() {
    g_current = "eval_schedule";
    auto S = [](std::string_view e) { return OH(e); };

    { auto o = S("24/7"); check_schedule(o, "2020-06-01", "00:00 open 24:00"); }
    // Time span
    { auto o = S("14:00-19:00"); check_schedule(o, "2020-06-01", "14:00 open 19:00"); }
    { auto o = S("Mo 14:00-19:00"); check_schedule(o, "2020-06-01", "14:00 open 19:00"); }
    { auto o = S("Su 14:00-19:00"); check_schedule_empty(o, "2020-06-01"); }
    { auto o = S("Su 14:00-25:30"); check_schedule(o, "2020-06-01", "00:00 open 01:30"); }
    { auto o = S("10:00-12:00,11:00-16:00 unknown"); check_schedule(o, "2020-06-01", "10:00 unknown 16:00"); }
    { auto o = S("23:00-01:00"); check_schedule(o, "2020-06-01", "00:00 open 01:00 | 23:00 open 24:00"); }
    { auto o = S("Mo 04:00-04:00"); check_schedule_empty(o, "2025-02-23"); }
    { auto o = S("Mo 04:00-04:00"); check_schedule(o, "2025-02-24", "04:00 open 24:00"); }
    { auto o = S("Mo 04:00-04:00"); check_schedule(o, "2025-02-25", "00:00 open 04:00"); }
    { auto o = S("Mo 04:00-04:00"); check_schedule_empty(o, "2025-02-26"); }
    { auto o = S("10:00-12:00,14:00-16:00"); check_schedule(o, "2020-06-01", "10:00 open 12:00 | 14:00 open 16:00"); }
    { auto o = S("10:00-12:00,14:00-25:30"); check_schedule(o, "2020-06-01", "00:00 open 01:30 | 10:00 open 12:00 | 14:00 open 24:00"); }
    // Weekday range
    { auto o = S("Mo-Su"); check_schedule(o, "2020-06-01", "00:00 open 24:00"); }
    { auto o = S("Tu"); check_schedule(o, "2020-06-02", "00:00 open 24:00"); }
    { auto o = S("We"); check_schedule_empty(o, "2020-06-02"); }
    { auto o = S("Mo-Tu,Th,Sa-Su 10:00-12:00");
      check_schedule(o, "2020-06-01", "10:00 open 12:00");
      check_schedule(o, "2020-06-02", "10:00 open 12:00");
      check_schedule_empty(o, "2020-06-03");
      check_schedule(o, "2020-06-04", "10:00 open 12:00");
      check_schedule_empty(o, "2020-06-05");
      check_schedule(o, "2020-06-06", "10:00 open 12:00");
      check_schedule(o, "2020-06-07", "10:00 open 12:00"); }
    // Weekday nth
    { auto o = S("Mo[1] 10:00-12:00"); check_schedule(o, "2020-06-01", "10:00 open 12:00"); check_schedule_empty(o, "2020-06-08"); }
    { auto o = S("Mo[2-4] 10:00-12:00");
      check_schedule_empty(o, "2020-06-01");
      check_schedule(o, "2020-06-08", "10:00 open 12:00");
      check_schedule(o, "2020-06-15", "10:00 open 12:00");
      check_schedule(o, "2020-06-22", "10:00 open 12:00");
      check_schedule_empty(o, "2020-06-29"); }
    { auto o = S("Su[-1] 10:00-12:00");
      check_schedule_empty(o, "2020-06-21");
      check_schedule(o, "2020-06-28", "10:00 open 12:00");
      check_schedule_empty(o, "2020-06-07"); }
    { auto o = S("Mo[2-4] +2 days 10:00-12:00");
      check_schedule_empty(o, "2020-06-03");
      check_schedule(o, "2020-06-10", "10:00 open 12:00");
      check_schedule(o, "2020-06-24", "10:00 open 12:00");
      check_schedule_empty(o, "2020-07-01"); }
    { auto o = S("Mo[1] -1 days 10:00-12:00");
      check_schedule(o, "2020-05-31", "10:00 open 12:00");
      check_schedule_empty(o, "2020-06-01"); }
    // Week range
    { auto o = S("week01:10:00-12:00"); check_schedule(o, "2020-01-01", "10:00 open 12:00"); check_schedule_empty(o, "2020-01-06"); }
    { auto o = S("week01,22-23:10:00-12:00"); check_schedule(o, "2020-05-31", "10:00 open 12:00"); check_schedule(o, "2020-06-07", "10:00 open 12:00"); }
    { auto o = S("week01-53/2:10:00-12:00");
      check_schedule(o, "2020-01-01", "10:00 open 12:00");
      check_schedule_empty(o, "2020-01-08");
      check_schedule(o, "2020-01-15", "10:00 open 12:00");
      check_schedule_empty(o, "2020-01-22"); }
    // Month selector
    { auto o = S("2020Jun01:10:00-12:10"); check_schedule(o, "2020-06-01", "10:00 open 12:10"); }
    { auto o = S("Jan-Jun:11:58-11:59"); check_schedule(o, "2020-06-01", "11:58 open 11:59"); }
    { auto o = S("May15-01:10:00-12:00"); check_schedule(o, "2020-06-01", "10:00 open 12:00"); check_schedule_empty(o, "2020-06-02"); }
    { auto o = S("2019Sep01-2020Jul31:10:00-12:00"); check_schedule(o, "2020-06-01", "10:00 open 12:00"); }
    { auto o = S("Sep01-Jul01:10:00-12:00"); check_schedule(o, "2020-06-01", "10:00 open 12:00"); }
    // Month OOB
    { auto o = S("Feb01-Feb31:10:00-12:00");
      check_schedule_empty(o, "2020-01-31");
      check_schedule(o, "2020-02-01", "10:00 open 12:00");
      check_schedule(o, "2020-02-29", "10:00 open 12:00");
      check_schedule_empty(o, "2020-03-01"); }
    // Month with weekday
    { auto o = S("Feb Mo[2]-Sep Su[-1] 10:00-12:00");
      check_schedule_empty(o, "2020-01-01");
      check_schedule(o, "2020-06-01", "10:00 open 12:00"); }
    // Year selector
    { auto o = S("2020:10:00-12:00"); check_schedule(o, "2020-01-01", "10:00 open 12:00"); check_schedule_empty(o, "2021-01-01"); }
    { auto o = S("2010-2019,2021,2025+:10:00-12:00");
      check_schedule_empty(o, "2020-01-01");
      check_schedule(o, "2015-01-01", "10:00 open 12:00");
      check_schedule(o, "5742-01-01", "10:00 open 12:00"); }
    { auto o = S("2010-2100/3:10:00-12:00");
      check_schedule(o, "2010-01-01", "10:00 open 12:00");
      check_schedule(o, "2019-01-01", "10:00 open 12:00");
      check_schedule_empty(o, "2017-01-01"); }
    // Rule normal
    { auto o = S("Jun ; 00:00-04:00 closed"); check_schedule(o, "2020-06-01", "04:00 open 24:00"); check_schedule_empty(o, "2020-07-01"); }
    { auto o = S("Sa,Su 11:00-13:45 open; 10:00-18:00"); check_schedule(o, "2020-06-01", "10:00 open 18:00"); check_schedule(o, "2020-05-31", "10:00 open 18:00"); }
    // Rule additional
    { auto o = S("10:00-12:00 open, 14:00-16:00 unknown, 16:00-23:00 closed"); check_schedule(o, "2020-06-01", "10:00 open 12:00 | 14:00 unknown 16:00"); }
    { auto o = S("10:00-20:00 open, 12:00-14:00 closed"); check_schedule(o, "2020-06-01", "10:00 open 12:00 | 14:00 open 20:00"); }
    { auto o = S("12:00-14:00 closed, 10:00-20:00 open"); check_schedule(o, "2020-06-01", "10:00 open 20:00"); }
    // Rule fallback
    { auto o = S("Jun:10:00-12:00 open || unknown"); check_schedule(o, "2020-06-01", "10:00 open 12:00"); check_schedule(o, "2020-05-31", "00:00 unknown 24:00"); }
    { auto o = S("Jun:10:00-12:00 open || Mo-Fr closed || unknown");
      check_schedule(o, "2020-06-01", "10:00 open 12:00");
      check_schedule(o, "2020-05-29", "00:00 unknown 24:00");
      check_schedule(o, "2020-05-30", "00:00 unknown 24:00"); }
    // Comments
    { auto o = S(R"(10:00-12:00 open "welcome!")"); check_schedule(o, "2020-06-01", "10:00 open[welcome!] 12:00"); }
    { auto o = S(R"(10:00-18:00 "may close later" ; 12:00-13:00 closed "ring the bell")");
      check_schedule(o, "2024-01-01", "10:00 open[may close later] 12:00 closed[ring the bell] 13:00 open[may close later] 18:00"); }
    { auto o = S(R"(10:00-18:00 "may close later", 12:00-13:00 "ring the bell")");
      check_schedule(o, "2024-01-01", "10:00 open[may close later] 12:00 open[ring the bell] 13:00 open[may close later] 18:00"); }
    { auto o = S(R"(10:00-14:00 "may open earlier", 14:00-18:00 "may close later")");
      check_schedule(o, "2024-01-01", "10:00 open[may open earlier] 14:00 open[may close later] 18:00"); }
    // Time events (fixed fallback, NoLocation): dawn 06:00 sunrise 07:00 sunset 19:00 dusk 20:00
    { auto o = S("(dawn-02:30)-(dusk+02:30)"); check_schedule(o, "2020-06-01", "03:30 open 22:30"); }
    { auto o = S("(dawn+00:30)-(dusk-00:30)"); check_schedule(o, "2020-06-01", "06:30 open 19:30"); }
    { auto o = S("sunrise-19:45"); check_schedule(o, "2020-06-01", "07:00 open 19:45"); }
    { auto o = S("08:15-sunset"); check_schedule(o, "2020-06-01", "08:15 open 19:00"); }
}

// ===========================================================================
// eval_next_change.rs
// ===========================================================================
static void test_eval_next_change() {
    g_current = "eval_next_change";
    check_next_change("1789-07-14 12:00", "24/7", "1900-01-01 00:00");
    check_next_change("1789-07-14 12:00", "3000", "3000-01-01 00:00");
    check_next_change("2024-06-21 22:30", "Jun dusk+", "2024-06-22 00:00");
    // Year ranges
    check_next_change("2021-02-09 21:00", "2000-3000", "3001-01-01 00:00");
    check_next_change("2021-02-09 21:00", "2000-3000/42", "2042-01-01 00:00");
    check_next_change("2021-02-09 21:00", "2000-3000/21", "2022-01-01 00:00");
    check_next_change("2021-02-09 21:00", "2020,8000-9000 10:00-22:00", "8000-01-01 10:00");
    // Week range
    check_next_change("7569-12-28 08:05", "week 52 ; Jun", "7569-12-29 00:00");
    check_next_change("7569-12-28 08:05", "week 1 ; Jun", "7569-12-29 00:00");
    check_next_change("2021-12-28 08:05", "week 52 ; Jun", "2022-01-03 00:00");
    check_next_change("2020-12-28 08:05", "week 53 ; Jun", "2021-01-04 00:00");
    check_next_change("2021-01-15 08:05", "week 53", "2026-12-28 00:00");
    // Month selector
    check_next_change("2024-02-15 10:00", "Jun", "2024-06-01 00:00");
    check_next_change("2024-06-15 10:00", "Jun", "2024-07-01 00:00");
    check_next_change("2021-02-15 12:00", "2021 Mar 28-Apr 16", "2021-03-28 00:00");
    check_next_change("2021-04-01 12:00", "2021 Mar 28-Apr 16", "2021-04-17 00:00");
    check_next_change("2021-02-15 12:00", "Mar 28-2021 Apr 16", "2021-03-28 00:00");
    check_next_change("2021-04-01 12:00", "Mar 28-2021 Apr 16", "2021-04-17 00:00");
    check_next_change("2024-03-14 12:00", "Feb 29", "2028-02-29 00:00");
    // Month selector with weekday
    check_next_change("2020-01-01 12:00", "Jan Mo[1]-Jan Su[-1]", "2020-01-06 00:00");
    check_next_change("2020-01-06 12:00", "Jan Mo[1]-Jan Su[-1]", "2020-01-27 00:00");
    check_next_change("2020-01-27 12:00", "Jan Mo[1]-Jan Su[-1]", "2021-01-04 00:00");
    check_next_change("2020-01-01 12:00", "easter-2025 Jan Su[-2]", "2024-03-31 00:00");
    check_next_change("2024-03-31 12:00", "easter-2025 Jan Su[-2]", "2025-01-20 00:00");
    check_next_change("2020-01-01 12:00", "Jan Mo[1]-15", "2020-01-06 00:00");
    check_next_change("2020-01-06 12:00", "Jan Mo[1]-15", "2020-01-16 00:00");
    check_next_change("2020-01-16 12:00", "Jan Mo[1]-15", "2021-01-04 00:00");
    // Comment-only changes
    check_next_change("2024-01-01 12:00", R"("aaa" ; Mar)", "2024-03-01 00:00");
    check_next_change("2024-03-15 12:00", R"("aaa" ; Mar)", "2024-04-01 00:00");
    check_next_change("2024-01-01 12:00", R"(00:00-14:00 "may open earlier", 14:00-24:00)", "2024-01-01 14:00");
    check_next_change("2024-01-01 16:00", R"(00:00-14:00 "may open earlier", 14:00-24:00)", "2024-01-02 00:00");
    check_next_change("2024-01-01 12:00", R"(24/7 "aaa" ; Mar "bbb")", "2024-03-01 00:00");
    check_next_change("2024-03-15 12:00", R"(24/7 "aaa" ; Mar "bbb")", "2024-04-01 00:00");
    check_next_change("2024-01-01 02:00", R"(01:00-03:00 closed "aaa")", "2024-01-01 03:00");
    check_next_change("2024-01-01 12:00", R"(01:00-03:00 closed "aaa")", "2024-01-02 01:00");

    // No next change
    check_no_next_change("9999-01-01 12:00", "24/7");
    check_no_next_change("2019-02-10 11:00", "24/7");
    check_no_next_change("+10000-01-01 00:00", "24/7");
    check_no_next_change("2021-04-17 12:00", "2021 Mar 28-Apr 16");
    check_no_next_change("2021-04-17 12:00", "Mar 28-2021 Apr 16");
    check_no_next_change("2025-01-20 12:00", "easter-2025 Jan Su[-2]");
    check_no_next_change("2020-01-01 12:00", "2020 Jan Mo[-1]-15");
    check_no_next_change("2020-01-01 12:00", "2020 Jan Mo[5]-15");
}

// ===========================================================================
// Holidays (Milestone 7). We insert the specific holiday dates the Rust
// country tests rely on, rather than porting the bundled DB.
// ===========================================================================
static std::shared_ptr<CompactCalendar> cal_of(std::initializer_list<NaiveDate> dates) {
    auto cal = std::make_shared<CompactCalendar>();
    for (auto d : dates) cal->insert(d);
    return cal;
}

static void test_holidays() {
    g_current = "holidays";

    // Empty holiday context: "PH off" degrades to never-off.
    {
        auto oh = OH("10:00-12:00; PH off");
        check_schedule(oh, "2020-07-14", "10:00 open 12:00");
    }

    // FR: 14th of July is a public holiday.
    {
        Context<NoLocation> ctx;
        ctx.holidays = ContextHolidays{cal_of({NaiveDate::ymd_unchecked(2020, 7, 14)}), nullptr};
        auto oh = OH("10:00-12:00; PH off").with_context(ctx);
        check_schedule_empty(oh, "2020-07-14");
    }
    // US: not a holiday on the 14th -> open.
    {
        Context<NoLocation> ctx;
        ctx.holidays = ContextHolidays{cal_of({NaiveDate::ymd_unchecked(2020, 7, 3)}), nullptr};
        auto oh = OH("10:00-12:00; PH off").with_context(ctx);
        check_schedule(oh, "2020-07-14", "10:00 open 12:00");
        // Independence Day observed on Fri Jul 3.
        check_schedule_empty(oh, "2020-07-03");
        check_schedule(oh, "2020-07-04", "10:00 open 12:00");
    }
    // DE Berlin: Women's Day is a regional (unknown) holiday -> Unknown.
    {
        Context<NoLocation> ctx;
        ctx.holidays_unknown = ContextHolidays{cal_of({NaiveDate::ymd_unchecked(2025, 3, 8)}), nullptr};
        auto oh = OH("10:00-12:00; PH off").with_context(ctx);
        check_schedule(oh, "2025-03-08", "00:00 unknown 24:00");
        auto oh2 = OH("08:00-18:00, PH 12:00-14:00 off").with_context(ctx);
        check_schedule(oh2, "2025-03-08", "08:00 open 12:00 unknown 14:00 open 18:00");
    }
}

// ===========================================================================
// Timezone providers (test fixtures only — the library binds no tz DB).
// ===========================================================================

// A constant-offset zone.
struct FixedOffset {
    int offset_seconds = 0;
    NaiveDateTime to_local(int64_t utc) const {
        int64_t local = utc + offset_seconds;
        int64_t days = floordiv(local, 86400);
        int64_t sod = local - days * 86400;
        return NaiveDateTime{std::chrono::sys_days{std::chrono::days{days}}, int(sod / 60)};
    }
    LocalResolution from_local(NaiveDateTime local) const {
        int64_t L = local.date().day_number() * 86400 + int64_t(local.minute_) * 60;
        return LocalResolution::single(L - offset_seconds);
    }
    bool operator==(const FixedOffset&) const = default;
};

// Europe/Paris: CET (+1) / CEST (+2), EU DST rule (last Sunday of Mar/Oct at
// 01:00 UTC). Enough to exercise the spring-forward gap & fall-back ambiguity.
struct EuroParis {
    static int64_t last_sunday_0100_utc(int year, unsigned month) {
        NaiveDate d = NaiveDate::ymd_unchecked(
            year, month, count_days_in_month(NaiveDate::ymd_unchecked(year, month, 1)));
        while (d.weekday() != Weekday::Sun) d = d.pred();
        return d.day_number() * 86400 + 3600;
    }
    int offset(int64_t utc) const {
        int year = NaiveDate{std::chrono::sys_days{std::chrono::days{floordiv(utc, 86400)}}}.year();
        int64_t spring = last_sunday_0100_utc(year, 3);
        int64_t fall = last_sunday_0100_utc(year, 10);
        return (utc >= spring && utc < fall) ? 7200 : 3600;
    }
    NaiveDateTime to_local(int64_t utc) const {
        int64_t local = utc + offset(utc);
        int64_t days = floordiv(local, 86400);
        int64_t sod = local - days * 86400;
        return NaiveDateTime{std::chrono::sys_days{std::chrono::days{days}}, int(sod / 60)};
    }
    LocalResolution from_local(NaiveDateTime local) const {
        int64_t L = local.date().day_number() * 86400 + int64_t(local.minute_) * 60;
        int64_t u1 = L - 3600;
        int64_t u2 = L - 7200;
        bool v1 = offset(u1) == 3600;
        bool v2 = offset(u2) == 7200;
        if (v1 && v2) return LocalResolution::ambiguous(std::min(u1, u2), std::max(u1, u2));
        if (v1) return LocalResolution::single(u1);
        if (v2) return LocalResolution::single(u2);
        return LocalResolution::none();
    }
    bool operator==(const EuroParis&) const = default;
};

template <class Tz>
static ZonedDateTime zdt(const Tz& tz, std::string_view s) {
    NaiveDateTime naive = pdt(s);
    auto r = tz.from_local(naive);
    return ZonedDateTime{r.latest};
}

// ===========================================================================
// eval_schedule.rs — timezone (sun events) module
// ===========================================================================
static void test_schedule_with_tz() {
    g_current = "eval_schedule/tz";
    auto paris = Coordinates::make(48.87, 2.29).value();
    auto antartica = Coordinates::make(-80.0, 80.0).value();

    auto paris_ctx = Context<NoLocation>{}.with_locale(TzLocation<EuroParis>{EuroParis{}, paris});
    // Antarctica (-80, 80) resolves to a fixed UTC+7 zone.
    auto anta_ctx = Context<NoLocation>{}.with_locale(TzLocation<FixedOffset>{FixedOffset{7 * 3600}, antartica});

    auto P = [&](std::string_view e) { return OH(e).with_context(paris_ctx); };
    auto A = [&](std::string_view e) { return OH(e).with_context(anta_ctx); };

    { auto o = P("sunrise-19:45"); check_schedule(o, "2020-06-01", "05:51 open 19:45"); }
    { auto o = P("08:15-sunset"); check_schedule(o, "2020-06-01", "08:15 open 21:46"); }
    { auto o = P("(dawn+00:30)-(dusk-00:30)"); check_schedule(o, "2020-06-01", "05:40 open 21:57"); }
    { auto o = P("(dawn-02:30)-(dusk+02:30)"); check_schedule(o, "2020-06-01", "00:00 open 00:56 | 02:40 open 24:00"); }

    // At the poles, sun events may not happen (seasonal fallback).
    { auto o = A("sunrise-20:00"); check_schedule(o, "2020-04-01", "09:11 open 20:00"); }
    { auto o = A("10:00-sunset"); check_schedule(o, "2020-04-01", "10:00 open 18:15"); }
    { auto o = A("sunset-dusk"); check_schedule(o, "2020-04-01", "18:15 open 20:16"); }
    { auto o = A("dusk-sunset"); check_schedule(o, "2020-04-01", "00:00 open 18:25 | 20:16 open 24:00"); }
    // The sun is already set in winter.
    { auto o = A("sunrise-20:00"); check_schedule_empty(o, "2020-01-01"); }
    { auto o = A("10:00-sunset"); check_schedule_empty(o, "2020-01-01"); }
    { auto o = A("sunset-dusk"); check_schedule_empty(o, "2020-01-01"); }
    { auto o = A("dusk-sunset"); check_schedule(o, "2020-01-01", "00:00 open 24:00"); }
    // The sun never sets in summer.
    { auto o = A("sunrise-20:00"); check_schedule(o, "2020-06-01", "00:00 open 20:00"); }
    { auto o = A("10:00-sunset"); check_schedule(o, "2020-06-01", "10:00 open 24:00"); }
    { auto o = A("sunset-dusk"); check_schedule(o, "2020-06-01", "00:00 open 24:00"); }
    { auto o = A("dusk-sunset"); check_schedule_empty(o, "2020-06-01"); }
}

// ===========================================================================
// localization.rs — timezone / DST behavior
// ===========================================================================
static void test_localization_tz() {
    g_current = "localization/tz";
    EuroParis tz;
    auto ctx = Context<NoLocation>{}.with_locale(TzLocation<EuroParis>{tz});

    // Plain tz: next change lands on a normal boundary.
    {
        auto oh = OH("10:00-18:00").with_context(ctx);
        auto nc = oh.next_change(zdt(tz, "2024-12-23 14:44"));
        CHECK(nc.has_value() && *nc == zdt(tz, "2024-12-23 18:00"), "ctx_with_tz");
    }
    // Spring-forward: 02:30 does not exist -> snaps to 03:00.
    {
        auto oh = OH("10:00-26:30").with_context(ctx);
        auto nc = oh.next_change(zdt(tz, "2024-03-30 14:44"));
        CHECK(nc.has_value() && *nc == zdt(tz, "2024-03-31 03:00"), "ends_at_invalid_time");
    }
    // Fall-back: end at 02:30 of the following (CET) day.
    {
        auto oh = OH("10:00-26:30").with_context(ctx);
        auto nc = oh.next_change(zdt(tz, "2024-10-27 14:44"));
        CHECK(nc.has_value() && *nc == zdt(tz, "2024-10-28 02:30"), "ends_at_ambiguous_time");
    }

    // Sun events with inferred-tz-equivalent coordinates.
    {
        auto paris = Coordinates::make(48.8535, 2.34839).value();
        auto sun_ctx = Context<NoLocation>{}.with_locale(TzLocation<EuroParis>{tz, paris});
        auto oh = OH("sunrise-sunset").with_context(sun_ctx);
        auto nc = oh.next_change(zdt(tz, "2024-12-23 14:44"));
        CHECK(nc.has_value() && *nc == zdt(tz, "2024-12-23 16:57"), "infer_tz sunset");

        // With a public holiday on 2024-07-14, reopen at sunrise on the 15th.
        auto hol_ctx = sun_ctx;
        hol_ctx.holidays = ContextHolidays{cal_of({NaiveDate::ymd_unchecked(2024, 7, 14)}), nullptr};
        auto oh2 = OH("sunrise-sunset; PH off").with_context(hol_ctx);
        auto nc2 = oh2.next_change(zdt(tz, "2024-07-14 14:44"));
        CHECK(nc2.has_value() && *nc2 == zdt(tz, "2024-07-15 06:03"), "infer_all sunrise after PH");
    }
}

int main() {
    test_eval_state();
    test_eval_schedule();
    test_eval_next_change();
    test_holidays();
    test_schedule_with_tz();
    test_localization_tz();

    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
