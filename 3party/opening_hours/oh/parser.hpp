// opening_hours.hpp – Self-contained C++23 OSM opening_hours parser
// No external dependencies.  Three-pass pipeline:
//   Pass 1  – normalize_input()  : fix common non-standard variants
//   Pass 2  – parse_normalized() : recursive-descent parse
//   Pass 3  – dedup()            : deduplicate / remove redundant data
//
// Top-level convenience: parse(sv) runs all three passes.
// Grammar reference: https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification

#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <expected>
#include <functional>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace opening_hours {

// ============================================================================
// Forward declarations
// ============================================================================
struct OpeningHoursExpression;

// ============================================================================
// Utility
// ============================================================================
inline std::string fmt02(int n) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d", n);
    return buf;
}

// ============================================================================
// Enums
// ============================================================================

enum class Weekday : uint8_t { Mon = 0, Tue, Wed, Thu, Fri, Sat, Sun };

constexpr int kNumWeekdays = 7;

constexpr Weekday weekday_from_index(int i) {
    return static_cast<Weekday>(((i % kNumWeekdays) + kNumWeekdays) % kNumWeekdays);
}

constexpr Weekday next_weekday(Weekday d) {
    return weekday_from_index(static_cast<int>(d) + 1);
}

constexpr const char* weekday_short(Weekday d) {
    constexpr const char* tbl[] = {"Mo","Tu","We","Th","Fr","Sa","Su"};
    return tbl[static_cast<int>(d)];
}

enum class Month : uint8_t {
    Jan=1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
};

constexpr Month next_month(Month m) {
    return static_cast<Month>((static_cast<int>(m) % 12) + 1);
}

constexpr Month prev_month(Month m) {
    return static_cast<Month>(((static_cast<int>(m) + 10) % 12) + 1);
}

constexpr const char* month_short(Month m) {
    constexpr const char* tbl[] = {
        "", "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    return tbl[static_cast<int>(m)];
}

enum class TimeEvent : uint8_t { Dawn, Sunrise, Sunset, Dusk };

constexpr const char* time_event_str(TimeEvent e) {
    constexpr const char* tbl[] = {"dawn","sunrise","sunset","dusk"};
    return tbl[static_cast<int>(e)];
}

enum class RuleKind : uint8_t { Open, Closed, Unknown };

constexpr const char* rule_kind_str(RuleKind k) {
    constexpr const char* tbl[] = {"open","closed","unknown"};
    return tbl[static_cast<int>(k)];
}

enum class RuleOperator : uint8_t { Normal, Additional, Fallback };

enum class HolidayKind : uint8_t { Public, School };

constexpr const char* holiday_kind_str(HolidayKind k) {
    return k == HolidayKind::Public ? "PH" : "SH";
}

enum class WeekDayOffsetKind : uint8_t { None, Next, Prev };

// ============================================================================
// ExtendedTime  –  00:00 .. 48:00
// ============================================================================

struct ExtendedTime {
    uint8_t hour = 0;
    uint8_t minute = 0;

    constexpr ExtendedTime() = default;
    constexpr ExtendedTime(uint8_t h, uint8_t m) : hour(h), minute(m) {}

    static constexpr std::optional<ExtendedTime> make(uint8_t h, uint8_t m) {
        if (h > 48 || m > 59 || (h == 48 && m > 0)) return std::nullopt;
        return ExtendedTime{h, m};
    }

    static constexpr ExtendedTime midnight_00() { return {0, 0}; }
    static constexpr ExtendedTime midnight_24() { return {24, 0}; }
    static constexpr ExtendedTime midnight_48() { return {48, 0}; }

    constexpr uint16_t mins_from_midnight() const {
        return static_cast<uint16_t>(hour) * 60 + minute;
    }

    static constexpr std::optional<ExtendedTime> from_mins(uint16_t m) {
        return make(static_cast<uint8_t>(m / 60), static_cast<uint8_t>(m % 60));
    }

    constexpr auto operator<=>(const ExtendedTime&) const = default;

    std::string to_string() const {
        return fmt02(hour) + ":" + fmt02(minute);
    }
};

// ============================================================================
// Time types
// ============================================================================

struct VariableTime {
    TimeEvent event = TimeEvent::Dawn;
    int16_t offset = 0; // minutes

    auto operator<=>(const VariableTime&) const = default;

    std::string to_string() const {
        std::string s(time_event_str(event));
        if (offset == 0) return s;
        // Match the reference grammar: parenthesized, HH:MM offset,
        // e.g. "(sunrise-00:10)", "(sunset+01:15)".
        int a = offset < 0 ? -offset : offset;
        return "(" + s + (offset < 0 ? '-' : '+') + fmt02(a / 60) + ":" + fmt02(a % 60) + ")";
    }
};

struct Time {
    enum Tag { Fixed, Variable } tag = Fixed;
    ExtendedTime fixed{};
    VariableTime variable{};

    static Time make_fixed(ExtendedTime t) { return {Fixed, t, {}}; }
    static Time make_variable(VariableTime v) { return {Variable, {}, v}; }

    auto operator<=>(const Time&) const = default;

    std::string to_string() const {
        return tag == Fixed ? fixed.to_string() : variable.to_string();
    }
};

struct TimeSpan {
    Time start{};
    Time end{};
    bool open_end = false;
    std::optional<int16_t> repeats_minutes; // total minutes for the repeat interval

    static TimeSpan fixed_range(ExtendedTime s, ExtendedTime e) {
        return {Time::make_fixed(s), Time::make_fixed(e), false, std::nullopt};
    }

    auto operator<=>(const TimeSpan&) const = default;

    std::string to_string() const {
        std::string s = start.to_string();
        if (!open_end || end != Time::make_fixed(ExtendedTime::midnight_24())) {
            s += "-" + end.to_string();
        }
        if (open_end) s += "+";
        if (repeats_minutes) {
            int rm = *repeats_minutes;
            int rh = rm / 60;
            int rmm = rm % 60;
            if (rh > 0) s += "/" + fmt02(rh) + ":" + fmt02(rmm);
            else s += "/" + fmt02(rmm);
        }
        return s;
    }
};

struct TimeSelector {
    std::vector<TimeSpan> time;

    TimeSelector() : time{TimeSpan::fixed_range(ExtendedTime::midnight_00(),
                                                  ExtendedTime::midnight_24())} {}
    explicit TimeSelector(std::vector<TimeSpan> t) {
        if (t.empty())
            time = {TimeSpan::fixed_range(ExtendedTime::midnight_00(),
                                           ExtendedTime::midnight_24())};
        else
            time = std::move(t);
    }

    bool is_00_24() const {
        return time.size() == 1
            && time[0] == TimeSpan::fixed_range(ExtendedTime::midnight_00(),
                                                  ExtendedTime::midnight_24());
    }

    bool operator==(const TimeSelector&) const = default;

    std::string to_string() const {
        std::string s;
        for (size_t i = 0; i < time.size(); ++i) {
            if (i) s += ",";
            s += time[i].to_string();
        }
        return s;
    }
};

// ============================================================================
// Day types
// ============================================================================

// --- Year ---

struct YearRange {
    uint16_t start = 0;
    uint16_t end = 0;
    uint16_t step = 1;

    auto operator<=>(const YearRange&) const = default;

    std::string to_string() const {
        std::string s = std::to_string(start);
        // '/' step is only valid on an actual range, so nest it under start!=end.
        if (start != end) {
            s += "-" + std::to_string(end);
            if (step != 1) s += "/" + std::to_string(step);
        }
        return s;
    }
};

// --- Week ---

struct WeekRange {
    uint8_t start = 0;
    uint8_t end = 0;
    uint8_t step = 1;

    auto operator<=>(const WeekRange&) const = default;

    std::string to_string() const {
        std::string s = fmt02(start);
        // Collapse a single week to just "NN" (drop "-NN" and any "/step").
        if (start != end) {
            s += "-" + fmt02(end);
            if (step != 1) s += "/" + std::to_string(step);
        }
        return s;
    }
};

// --- Date ---

struct DateOffset {
    WeekDayOffsetKind wday_kind = WeekDayOffsetKind::None;
    Weekday wday = Weekday::Mon;
    int64_t day_offset = 0;

    auto operator<=>(const DateOffset&) const = default;

    std::string to_string() const {
        std::string s;
        switch (wday_kind) {
        case WeekDayOffsetKind::None: break;
        case WeekDayOffsetKind::Next: s += std::string("+") + weekday_short(wday); break;
        case WeekDayOffsetKind::Prev: s += std::string("-") + weekday_short(wday); break;
        }
        if (day_offset != 0) {
            s += " ";
            if (day_offset > 0) s += "+";
            s += std::to_string(day_offset) + " day";
            if (std::abs(day_offset) > 1) s += "s";
        }
        return s;
    }
};

enum class DateKind : uint8_t { Fixed, WeekdayInMonth, Easter };

struct Date {
    DateKind kind = DateKind::Fixed;
    std::optional<uint16_t> year;
    Month month = Month::Jan;
    uint8_t day = 1;        // Fixed only
    Weekday weekday = Weekday::Mon; // WeekdayInMonth only
    int8_t nth = 0;         // WeekdayInMonth only

    static Date make_fixed(std::optional<uint16_t> y, Month m, uint8_t d) {
        return {DateKind::Fixed, y, m, d, {}, 0};
    }
    static Date make_weekday_in_month(std::optional<uint16_t> y, Month m, Weekday w, int8_t n) {
        return {DateKind::WeekdayInMonth, y, m, 0, w, n};
    }
    static Date make_easter(std::optional<uint16_t> y) {
        return {DateKind::Easter, y, Month::Jan, 0, {}, 0};
    }

    bool has_year() const { return year.has_value(); }

    auto operator<=>(const Date&) const = default;

    std::string to_string() const {
        std::string s;
        if (year) s += std::to_string(*year) + " ";
        switch (kind) {
        case DateKind::Fixed:
            s += std::string(month_short(month)) + " " + std::to_string(day);
            break;
        case DateKind::WeekdayInMonth:
            s += std::string(month_short(month)) + " " + weekday_short(weekday);
            if (nth != 0) s += "[" + std::to_string(nth) + "]";
            break;
        case DateKind::Easter:
            s += "easter";
            break;
        }
        return s;
    }
};

// --- MonthdayRange ---

enum class MonthdayRangeKind : uint8_t { MonthOnly, DateRange };

struct MonthdayRange {
    MonthdayRangeKind kind = MonthdayRangeKind::MonthOnly;
    // MonthOnly:
    std::optional<uint16_t> month_year;
    Month month_start = Month::Jan;
    Month month_end = Month::Jan;
    // DateRange:
    Date date_start{};
    DateOffset offset_start{};
    Date date_end{};
    DateOffset offset_end{};

    static MonthdayRange make_month(std::optional<uint16_t> y, Month s, Month e) {
        MonthdayRange r;
        r.kind = MonthdayRangeKind::MonthOnly;
        r.month_year = y;
        r.month_start = s;
        r.month_end = e;
        return r;
    }
    static MonthdayRange make_date(Date ds, DateOffset os, Date de, DateOffset oe) {
        MonthdayRange r;
        r.kind = MonthdayRangeKind::DateRange;
        r.date_start = ds;
        r.offset_start = os;
        r.date_end = de;
        r.offset_end = oe;
        return r;
    }

    auto operator<=>(const MonthdayRange&) const = default;

    std::string to_string() const {
        switch (kind) {
        case MonthdayRangeKind::MonthOnly: {
            std::string s;
            if (month_year) s += std::to_string(*month_year);
            s += month_short(month_start);
            if (month_start != month_end) s += std::string("-") + month_short(month_end);
            return s;
        }
        case MonthdayRangeKind::DateRange: {
            std::string s = date_start.to_string() + offset_start.to_string();
            if (!(date_start == date_end && offset_start == offset_end)) {
                s += "-" + date_end.to_string() + offset_end.to_string();
            }
            return s;
        }
        }
        return {};
    }
};

// --- WeekDayRange ---

enum class WeekDayRangeKind : uint8_t { FixedDays, Holiday };

struct WeekDayRange {
    WeekDayRangeKind kind = WeekDayRangeKind::FixedDays;
    // FixedDays:
    Weekday range_start = Weekday::Mon;
    Weekday range_end = Weekday::Mon;
    int64_t offset = 0;
    bool nth_from_start[5] = {true,true,true,true,true};
    bool nth_from_end[5] = {true,true,true,true,true};
    // Holiday:
    HolidayKind holiday_kind = HolidayKind::Public;
    // (offset field is shared)

    static WeekDayRange make_fixed(Weekday s, Weekday e, int64_t off = 0) {
        WeekDayRange r;
        r.kind = WeekDayRangeKind::FixedDays;
        r.range_start = s;
        r.range_end = e;
        r.offset = off;
        return r;
    }
    static WeekDayRange make_holiday(HolidayKind hk, int64_t off = 0) {
        WeekDayRange r;
        r.kind = WeekDayRangeKind::Holiday;
        r.holiday_kind = hk;
        r.offset = off;
        return r;
    }

    bool nth_has_filter() const {
        for (int i = 0; i < 5; ++i) if (!nth_from_start[i]) return true;
        for (int i = 0; i < 5; ++i) if (!nth_from_end[i]) return true;
        return false;
    }

    auto operator<=>(const WeekDayRange&) const = default;

    std::string to_string() const {
        std::string s;
        switch (kind) {
        case WeekDayRangeKind::FixedDays:
            s += weekday_short(range_start);
            if (range_start != range_end) s += std::string("-") + weekday_short(range_end);
            if (nth_has_filter()) {
                s += "[";
                bool first = true;
                for (int i = 0; i < 5; ++i) {
                    if (nth_from_start[i]) {
                        if (!first) s += ",";
                        s += std::to_string(i + 1);
                        first = false;
                    }
                }
                for (int i = 0; i < 5; ++i) {
                    if (nth_from_end[i]) {
                        if (!first) s += ",";
                        s += std::to_string(-(i + 1));
                        first = false;
                    }
                }
                s += "]";
            }
            if (offset != 0) {
                s += " ";
                if (offset > 0) s += "+";
                s += std::to_string(offset) + " day";
                if (std::abs(offset) > 1) s += "s";
            }
            break;
        case WeekDayRangeKind::Holiday:
            s += holiday_kind_str(holiday_kind);
            if (offset != 0) {
                s += " ";
                if (offset > 0) s += "+";
                s += std::to_string(offset) + " day";
                if (std::abs(offset) > 1) s += "s";
            }
            break;
        }
        return s;
    }
};

// --- DaySelector ---

struct DaySelector {
    std::vector<YearRange> year;
    std::vector<MonthdayRange> monthday;
    std::vector<WeekRange> week;
    std::vector<WeekDayRange> weekday;

    bool is_empty() const {
        return year.empty() && monthday.empty() && week.empty() && weekday.empty();
    }

    bool operator==(const DaySelector&) const = default;

    std::string to_string(bool force = false) const {
        // An additional rule needs an explicit day selector, otherwise the
        // output re-parses as extra timespans of the previous rule (matches the
        // Rust lib, e.g. "Mo 10:00-12:00, Mo-Su 13:00-14:00").
        if (force && is_empty()) return "Mo-Su";
        std::string s;
        auto append_selector = [](std::string& out, const auto& vec) {
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i) out += ",";
                out += vec[i].to_string();
            }
        };
        if (!year.empty() || !monthday.empty() || !week.empty()) {
            append_selector(s, year);
            append_selector(s, monthday);
            if (!week.empty()) {
                if (!year.empty() || !monthday.empty()) s += " ";
                s += "week";
                std::string ws;
                append_selector(ws, week);
                s += ws;
            }
            if (!weekday.empty()) s += " ";
        }
        std::string wd;
        append_selector(wd, weekday);
        s += wd;
        return s;
    }
};

// ============================================================================
// Rule types
// ============================================================================

struct RuleSequence {
    DaySelector day_selector;
    TimeSelector time_selector;
    RuleKind kind = RuleKind::Open;
    RuleOperator op = RuleOperator::Normal;
    std::vector<std::string> comments;

    bool is_constant() const {
        return day_selector.is_empty() && time_selector.is_00_24();
    }

    bool operator==(const RuleSequence&) const = default;

    std::string to_string(bool force_day_selector = false) const {
        std::string s;
        bool is_empty_s = true;
        if (is_constant()) {
            s += "24/7";
            is_empty_s = false;
        } else {
            std::string ds = day_selector.to_string(force_day_selector);
            if (!ds.empty()) { s += ds; is_empty_s = false; }
            if (!time_selector.is_00_24()) {
                if (!is_empty_s) s += " ";
                s += time_selector.to_string();
                is_empty_s = false;
            }
        }
        if (kind != RuleKind::Open) {
            if (!is_empty_s) s += " ";
            s += rule_kind_str(kind);
            is_empty_s = false;
        }
        if (!comments.empty()) {
            if (!is_empty_s) s += " ";
            s += "\"";
            for (size_t i = 0; i < comments.size(); ++i) {
                if (i) s += ", ";
                s += comments[i];
            }
            s += "\"";
        }
        return s;
    }
};

struct OpeningHoursExpression {
    std::vector<RuleSequence> rules;

    std::string to_string() const {
        if (rules.empty()) return "closed";
        std::string s = rules[0].to_string();
        for (size_t i = 1; i < rules.size(); ++i) {
            switch (rules[i].op) {
            case RuleOperator::Normal:     s += "; ";   break;
            case RuleOperator::Additional: s += ", ";   break;
            case RuleOperator::Fallback:   s += " || "; break;
            }
            s += rules[i].to_string(rules[i].op == RuleOperator::Additional);
        }
        return s;
    }
};

// ============================================================================
// ============================================================================
// PASS 1 – Input Normalization (state-machine)
// ============================================================================
// ============================================================================

namespace detail {

// --- Keyword table ---
struct Keyword { const char* pattern; const char* canonical; };

// Ordered longest-first so first match wins.
inline constexpr Keyword kKeywords[] = {
    // German full weekday names
    {"donnerstag","Th"}, {"dienstag","Tu"}, {"mittwoch","We"},
    {"samstag","Sa"}, {"sonntag","Su"}, {"freitag","Fr"}, {"montag","Mo"},
    // English full weekday names
    {"wednesday","We"}, {"thursday","Th"}, {"saturday","Sa"},
    {"tuesday","Tu"}, {"monday","Mo"}, {"friday","Fr"}, {"sunday","Su"},
    // modifiers and events
    {"unknown","unknown"}, {"sunrise","sunrise"}, {"sunset","sunset"},
    {"closed","closed"}, {"open","open"}, {"dawn","dawn"}, {"dusk","dusk"},
    {"off","off"},
    // English 3-letter abbreviations
    {"mon","Mo"}, {"tue","Tu"}, {"wed","We"}, {"thu","Th"},
    {"fri","Fr"}, {"sat","Sa"}, {"sun","Su"},
    // month 3-letter
    {"jan","Jan"}, {"feb","Feb"}, {"mar","Mar"}, {"apr","Apr"},
    {"may","May"}, {"jun","Jun"}, {"jul","Jul"}, {"aug","Aug"},
    {"sep","Sep"}, {"oct","Oct"}, {"nov","Nov"}, {"dec","Dec"},
    // standard 2-letter weekdays
    {"mo","Mo"}, {"tu","Tu"}, {"we","We"}, {"th","Th"},
    {"fr","Fr"}, {"sa","Sa"}, {"su","Su"},
    // German 2-letter
    {"di","Tu"}, {"mi","We"}, {"do","Th"}, {"so","Su"},
    // holidays
    {"ph","PH"}, {"sh","SH"},
};

inline constexpr const char* kWeekdays[] = {"Mo","Tu","We","Th","Fr","Sa","Su"};

inline bool is_weekday_token(std::string_view s) {
    for (auto w : kWeekdays) if (s == w) return true;
    return false;
}

// Match a keyword at position pos in the character array.
// Returns (canonical, chars_consumed) or nullopt.
inline std::optional<std::pair<const char*, size_t>>
match_keyword(const std::vector<char>& chars, size_t pos) {
    size_t remaining = chars.size() - pos;
    for (auto& [pattern, canonical] : kKeywords) {
        size_t plen = std::strlen(pattern);
        if (remaining < plen) continue;
        bool ok = true;
        for (size_t k = 0; k < plen; ++k) {
            if (static_cast<char>(std::tolower(static_cast<unsigned char>(chars[pos + k])))
                != pattern[k]) { ok = false; break; }
        }
        if (!ok) continue;
        // word boundary: next char must not be alphabetic
        if (pos + plen < chars.size() && std::isalpha(static_cast<unsigned char>(chars[pos + plen])))
            continue;
        return std::pair{canonical, plen};
    }
    return std::nullopt;
}

// After emitting a weekday, look ahead for `SPACES - SPACES <weekday>`
// and return chars to skip up to (not including) the next weekday.
inline std::optional<size_t>
match_spaced_dash_weekday(const std::vector<char>& chars, size_t pos) {
    size_t len = chars.size();
    size_t j = pos;
    while (j < len && chars[j] == ' ') ++j;
    if (j >= len || chars[j] != '-') return std::nullopt;
    ++j;
    while (j < len && chars[j] == ' ') ++j;
    if (j + 1 < len) {
        char c0 = static_cast<char>(std::toupper(static_cast<unsigned char>(chars[j])));
        char c1 = static_cast<char>(std::tolower(static_cast<unsigned char>(chars[j + 1])));
        bool after_alpha = (j + 2 < len) && std::isalpha(static_cast<unsigned char>(chars[j + 2]));
        auto ok = [&](char a, char b) { return c0 == a && c1 == b && !after_alpha; };
        if (ok('M','o')||ok('T','u')||ok('W','e')||ok('T','h')||ok('F','r')||
            ok('S','a')||ok('S','u')||ok('D','i')||ok('M','i')||ok('D','o')||ok('S','o'))
            return j - pos;
    }
    return std::nullopt;
}

inline bool last_token_is_weekday(const std::string& result) {
    auto t = result;
    while (!t.empty() && t.back() == ' ') t.pop_back();
    if (t.size() < 2) return false;
    auto b1 = static_cast<unsigned char>(t[t.size()-1]);
    auto b2 = static_cast<unsigned char>(t[t.size()-2]);
    if (!std::isalpha(b1) || !std::isalpha(b2)) return false;
    std::string_view tail{t.data() + t.size() - 2, 2};
    if (!is_weekday_token(tail)) return false;
    return t.size() == 2 || !std::isalpha(static_cast<unsigned char>(t[t.size()-3]));
}

inline bool ends_with_time(const std::string& s) {
    auto t = s;
    while (!t.empty() && t.back() == ' ') t.pop_back();
    if (t.empty()) return false;
    if (t.back() == '+') return true;
    size_t n = t.size();
    if (n >= 3 && std::isdigit(static_cast<unsigned char>(t[n-1]))
               && std::isdigit(static_cast<unsigned char>(t[n-2]))
               && t[n-3] == ':') return true;
    return false;
}

inline bool should_insert_space_after_comma(const std::string& result) {
    if (result.empty()) return false;
    if (std::isdigit(static_cast<unsigned char>(result.back()))) return true;
    auto t = result;
    while (!t.empty() && t.back() == ' ') t.pop_back();
    if (t.ends_with("off") || t.ends_with("closed") || t.ends_with("open") || t.ends_with("unknown"))
        return true;
    return false;
}

// Convert AM/PM times.
inline std::optional<std::pair<std::string, size_t>>
try_match_ampm(const uint8_t* bytes, size_t start, size_t len) {
    size_t i = start;
    uint32_t h = bytes[i] - '0'; ++i;
    if (i < len && std::isdigit(bytes[i])) { h = h * 10 + (bytes[i] - '0'); ++i; }
    if (h == 0 || h > 12) return std::nullopt;
    if (i >= len || (bytes[i] != ':' && bytes[i] != '.')) return std::nullopt;
    ++i;
    if (i + 1 >= len || !std::isdigit(bytes[i]) || !std::isdigit(bytes[i+1])) return std::nullopt;
    char m0 = bytes[i], m1 = bytes[i+1]; i += 2;
    while (i < len && bytes[i] == ' ') ++i;
    if (i + 1 >= len) return std::nullopt;
    char ap = std::toupper(bytes[i]);
    char em = std::toupper(bytes[i+1]);
    if (em != 'M' || (ap != 'A' && ap != 'P')) return std::nullopt;
    size_t end_pos = i + 2;
    if (end_pos < len && std::isalpha(bytes[end_pos])) return std::nullopt;
    uint32_t h24 = (ap == 'P') ? (h == 12 ? 12 : h + 12) : (h == 12 ? 0 : h);
    char buf[8]; std::snprintf(buf, sizeof(buf), "%02u:%c%c", h24, m0, m1);
    return std::pair{std::string(buf), end_pos - start};
}

inline std::string pre_convert_ampm(std::string_view input) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(input.data());
    size_t len = input.size();
    std::string out; out.reserve(len);
    for (size_t i = 0; i < len; ) {
        if (std::isdigit(bytes[i]) && (i == 0 || !std::isdigit(bytes[i-1]))) {
            if (auto m = try_match_ampm(bytes, i, len)) {
                out += m->first; i += m->second; continue;
            }
        }
        out += static_cast<char>(bytes[i]); ++i;
    }
    return out;
}

// Convert French h-times: 8h30 → 08:30, 17h → 17:00
inline std::optional<std::pair<std::string, size_t>>
try_match_h_time(const uint8_t* bytes, size_t start, size_t len) {
    size_t i = start;
    uint32_t h = bytes[i] - '0'; ++i;
    if (i < len && std::isdigit(bytes[i])) { h = h * 10 + (bytes[i] - '0'); ++i; }
    if (i >= len || (bytes[i] != 'h' && bytes[i] != 'H')) return std::nullopt;
    ++i;
    if (i < len && std::isalpha(bytes[i])) return std::nullopt;
    char m0 = '0', m1 = '0';
    if (i + 1 < len && std::isdigit(bytes[i]) && std::isdigit(bytes[i+1])) {
        m0 = bytes[i]; m1 = bytes[i+1]; i += 2;
    }
    if (h > 24) return std::nullopt;
    uint32_t m = (m0 - '0') * 10 + (m1 - '0');
    if (m > 59 || (h == 24 && m > 0)) return std::nullopt;
    char buf[8]; std::snprintf(buf, sizeof(buf), "%02u:%c%c", h, m0, m1);
    return std::pair{std::string(buf), i - start};
}

inline std::string pre_convert_h_times(std::string_view input) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(input.data());
    size_t len = input.size();
    std::string out; out.reserve(len);
    for (size_t i = 0; i < len; ) {
        if (std::isdigit(bytes[i]) && (i == 0 || !std::isalnum(bytes[i-1]))) {
            if (auto m = try_match_h_time(bytes, i, len)) {
                out += m->first; i += m->second; continue;
            }
        }
        out += static_cast<char>(bytes[i]); ++i;
    }
    return out;
}

// Convert dot-format times: 8.00 → 8:00
inline std::string pre_convert_dot_times(std::string_view input) {
    std::string out(input);
    auto* bytes = reinterpret_cast<uint8_t*>(out.data());
    size_t len = out.size();
    for (size_t i = 1; i + 2 < len; ++i) {
        if (bytes[i] != '.') continue;
        if (!std::isdigit(bytes[i-1])) continue;
        bool has_h0 = (i >= 2 && std::isdigit(bytes[i-2]));
        if (has_h0 && i >= 3 && std::isdigit(bytes[i-3])) continue;
        if (!std::isdigit(bytes[i+1]) || !std::isdigit(bytes[i+2])) continue;
        if (i + 3 < len && (std::isdigit(bytes[i+3]) || bytes[i+3] == '.')) continue;
        uint32_t h = has_h0 ? (bytes[i-2]-'0')*10 + (bytes[i-1]-'0') : (bytes[i-1]-'0');
        uint32_t m = (bytes[i+1]-'0')*10 + (bytes[i+2]-'0');
        if (h <= 24 && m <= 59 && !(h == 24 && m > 0))
            bytes[i] = ':';
    }
    return out;
}

template <class F>
inline std::string normalize_unquoted(std::string_view input, F&& f) {
    std::string out;
    out.reserve(input.size());
    size_t segment_start = 0;
    bool in_quote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] != '"') continue;

        if (in_quote) {
            out += input.substr(segment_start, i - segment_start + 1);
            segment_start = i + 1;
            in_quote = false;
        } else {
            out += f(input.substr(segment_start, i - segment_start));
            segment_start = i;
            in_quote = true;
        }
    }

    if (in_quote) {
        out += input.substr(segment_start);
    } else {
        out += f(input.substr(segment_start));
    }

    return out;
}

inline constexpr const char* kMonthNames[] = {
    "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

// Post-processing: insert colon after comment label
inline std::string post_insert_comment_colon(std::string_view input) {
    std::string out; out.reserve(input.size() + 8);
    std::string_view rest = input;
    while (true) {
        auto q1 = rest.find('"');
        if (q1 == std::string_view::npos) { out += rest; break; }
        out += rest.substr(0, q1); out += '"';
        rest = rest.substr(q1 + 1);
        auto q2 = rest.find('"');
        if (q2 == std::string_view::npos) { out += rest; break; }
        out += rest.substr(0, q2); out += '"';
        rest = rest.substr(q2 + 1);
        if (rest.starts_with(' ') && !rest.starts_with(" :")) {
            auto after = rest.substr(1);
            bool is_oh = after.starts_with("Mo") || after.starts_with("Tu") ||
                after.starts_with("We") || after.starts_with("Th") ||
                after.starts_with("Fr") || after.starts_with("Sa") ||
                after.starts_with("Su") || after.starts_with("PH") ||
                after.starts_with("SH") ||
                (!after.empty() && std::isdigit(static_cast<unsigned char>(after[0])));
            if (is_oh) out += ':';
        }
    }
    return out;
}

// Post-processing: expand date comma lists  Dec 24,31 → Dec 24,Dec 31
inline std::string post_expand_date_comma_list(std::string_view input) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(input.data());
    size_t len = input.size();
    std::string out; out.reserve(len + 32);

    for (size_t i = 0; i < len; ) {
        // Look for Month pattern
        if (i + 3 <= len
            && std::isupper(bytes[i]) && std::islower(bytes[i+1]) && std::islower(bytes[i+2])) {
            std::string_view cand{input.data() + i, 3};
            bool is_month = false;
            for (auto mn : kMonthNames) if (cand == mn) { is_month = true; break; }
            if (is_month) {
                std::string month(cand);
                out += month; i += 3;
                if (i < len && bytes[i] == ' ') {
                    out += ' '; ++i;
                    size_t day_start = i;
                    while (i < len && std::isdigit(bytes[i])) { out += static_cast<char>(bytes[i]); ++i; }
                    if (i > day_start) {
                        while (i < len && bytes[i] == ',') {
                            size_t ac = i + 1, j = ac;
                            while (j < len && std::isdigit(bytes[j])) ++j;
                            size_t dl = j - ac;
                            if (dl >= 1 && dl <= 2 && (j >= len || bytes[j] != ':')
                                && (j >= len || !std::isalpha(bytes[j]))) {
                                out += ','; out += month; out += ' ';
                                i = ac;
                                while (i < len && std::isdigit(bytes[i])) { out += static_cast<char>(bytes[i]); ++i; }
                            } else break;
                        }
                    }
                }
                continue;
            }
        }
        out += static_cast<char>(bytes[i]); ++i;
    }
    return out;
}

} // namespace detail

// ---------------------------------------------------------------------------
// normalize_input  –  Pass 1
// ---------------------------------------------------------------------------
inline std::string normalize_input(std::string_view raw) {
    using namespace detail;

    // Strip a leading "opening_hours=" key that sometimes leaks into the value.
    {
        size_t p = 0;
        while (p < raw.size() && raw[p] == ' ') ++p;
        static constexpr std::string_view kKey = "opening_hours";
        if (raw.size() - p >= kKey.size()) {
            bool match = true;
            for (size_t k = 0; k < kKey.size(); ++k) {
                if (static_cast<char>(std::tolower(static_cast<unsigned char>(raw[p + k]))) != kKey[k]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                size_t q = p + kKey.size();
                while (q < raw.size() && raw[q] == ' ') ++q;
                if (q < raw.size() && raw[q] == '=')
                    raw = raw.substr(q + 1);
            }
        }
    }

    // Pre-processing: convert non-standard time formats
    std::string input = normalize_unquoted(raw, pre_convert_ampm);
    input = normalize_unquoted(input, pre_convert_h_times);
    input = normalize_unquoted(input, pre_convert_dot_times);

    std::vector<char> chars(input.begin(), input.end());
    size_t len = chars.size();
    std::string out; out.reserve(input.size());
    size_t i = 0;
    bool in_quote = false;

    while (i < len) {
        char c = chars[i];

        // Strip empty comments ""
        if (c == '"' && i + 1 < len && chars[i+1] == '"') {
            i += 2;
            while (i < len && chars[i] == ' ') ++i;
            continue;
        }

        // Quoted regions: copy verbatim
        if (c == '"') { in_quote = !in_quote; out += c; ++i; continue; }
        if (in_quote) { out += c; ++i; continue; }

        // Unicode dashes → ASCII hyphen
        // Check for multi-byte UTF-8 sequences for EN DASH (E2 80 93) and EM DASH (E2 80 94)
        if (i + 2 < len && static_cast<uint8_t>(chars[i]) == 0xE2
            && static_cast<uint8_t>(chars[i+1]) == 0x80
            && (static_cast<uint8_t>(chars[i+2]) == 0x93 || static_cast<uint8_t>(chars[i+2]) == 0x94)) {
            out += '-'; i += 3; continue;
        }
        // Full-width colon (EF BC 9A)
        if (i + 2 < len && static_cast<uint8_t>(chars[i]) == 0xEF
            && static_cast<uint8_t>(chars[i+1]) == 0xBC
            && static_cast<uint8_t>(chars[i+2]) == 0x9A) {
            out += ':'; i += 3; continue;
        }

        // Collapse runs of spaces
        if (c == ' ') {
            out += ' '; ++i;
            while (i < len && chars[i] == ' ') ++i;
            continue;
        }

        // Keyword normalisation at word boundary
        if (std::isalpha(static_cast<unsigned char>(c))
            && (out.empty() || !std::isalpha(static_cast<unsigned char>(out.back()))))
        {
            if (auto kw = match_keyword(chars, i)) {
                auto [canonical, advance] = *kw;
                // Insert "; " when weekday/holiday follows a time range
                if ((is_weekday_token(canonical) || std::strcmp(canonical,"PH")==0 || std::strcmp(canonical,"SH")==0)
                    && out.ends_with(' ') && ends_with_time(out))
                {
                    out.back() = ';';
                    out += ' ';
                }
                out += canonical;
                i += advance;
                if (is_weekday_token(canonical)) {
                    // skip trailing dot
                    if (i < len && chars[i] == '.') ++i;
                    // collapse spaced dash
                    if (auto skip = match_spaced_dash_weekday(chars, i)) {
                        out += '-'; i += *skip;
                    }
                    // insert space before digit
                    else if (i < len && std::isdigit(static_cast<unsigned char>(chars[i]))) {
                        out += ' ';
                    }
                }
                continue;
            }
        }

        // Comma directly before a keyword: insert space
        if (c == ',' && i + 1 < len && chars[i+1] != ' '
            && should_insert_space_after_comma(out))
        {
            if (std::isalpha(static_cast<unsigned char>(chars[i+1]))
                && match_keyword(chars, i + 1))
            {
                out += ','; out += ' '; ++i; continue;
            }
        }

        // "||" without trailing space
        if (c == '|' && i + 1 < len && chars[i+1] == '|') {
            out += "||"; i += 2;
            if (i < len && chars[i] != ' ') out += ' ';
            continue;
        }

        // Single pipe → semicolon
        if (c == '|') { out += ';'; ++i; continue; }

        // Colon after weekday → space
        if (c == ':' && last_token_is_weekday(out)) {
            if (!out.ends_with(' ')) out += ' ';
            ++i;
            while (i < len && chars[i] == ' ') ++i;
            continue;
        }

        // Collapse a spaced range dash " - " to "-" (months/years/dates/times:
        // "Apr - May", "2019 - 2090", "10:00 - 12:00"). Requires spaces on both
        // sides so negative day offsets like "PH -1 day" are left intact.
        if (c == '-' && !out.empty() && out.back() == ' '
            && i + 1 < len && chars[i + 1] == ' ') {
            while (!out.empty() && out.back() == ' ') out.pop_back();
            out += '-';
            ++i;
            while (i < len && chars[i] == ' ') ++i;
            continue;
        }

        out += c; ++i;
    }

    // Trim leading/trailing whitespace and trailing punctuation
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    while (!out.empty() && (out.back() == ';' || out.back() == '.')) {
        out.pop_back();
        while (!out.empty() && out.back() == ' ') out.pop_back();
    }

    // Collapse double semicolons
    auto collapse = [](std::string& s, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
        }
    };
    collapse(out, "; ;", ";");
    collapse(out, ";;", ";");

    // Post-processing
    out = detail::post_insert_comment_colon(out);
    out = detail::normalize_unquoted(out, detail::post_expand_date_comma_list);

    return out;
}

// ============================================================================
// ============================================================================
// PASS 2 – Recursive-descent parser
// ============================================================================
// ============================================================================

namespace detail {

class Parser {
public:
    explicit Parser(std::string_view input) : input_(input) {}

    std::expected<OpeningHoursExpression, std::string> parse() {
        if (input_.empty()) return std::unexpected("empty input");
        auto rules = parse_opening_hours();
        if (fatal_) return std::unexpected(std::move(error_));
        if (!rules) return std::unexpected(std::move(error_));
        skip_spaces();
        if (!at_end()) {
            return std::unexpected("unexpected trailing content at position " + std::to_string(pos_));
        }
        return OpeningHoursExpression{std::move(*rules)};
    }

private:
    std::string_view input_;
    size_t pos_ = 0;
    std::string error_;
    bool fatal_ = false;

    // -- Utility --
    bool at_end() const { return pos_ >= input_.size(); }
    char peek() const { return at_end() ? '\0' : input_[pos_]; }
    char peek(size_t off) const { auto p = pos_ + off; return p >= input_.size() ? '\0' : input_[p]; }
    void advance(size_t n = 1) { pos_ = std::min(pos_ + n, input_.size()); }
    bool match_char(char c) { if (peek() == c) { advance(); return true; } return false; }
    bool match_str(std::string_view s) {
        if (input_.substr(pos_).starts_with(s)) { advance(s.size()); return true; }
        return false;
    }
    bool match_str_ascii_ci(std::string_view s) {
        if (pos_ + s.size() > input_.size()) return false;
        for (size_t i = 0; i < s.size(); ++i) {
            unsigned char got = static_cast<unsigned char>(input_[pos_ + i]);
            unsigned char expected = static_cast<unsigned char>(s[i]);
            if (std::tolower(got) != std::tolower(expected)) return false;
        }
        advance(s.size());
        return true;
    }
    void skip_spaces() { while (peek() == ' ') advance(); }
    bool match_space() { return match_char(' '); }

    size_t save() const { return pos_; }
    void restore(size_t p) { pos_ = p; }

    void set_error(const std::string& msg) {
        if (error_.empty())
            error_ = msg + " at position " + std::to_string(pos_);
    }

    // A fatal error aborts the whole parse even when the offending text is
    // otherwise consumable (a valid-looking but semantically invalid range,
    // e.g. an inverted year/week range).
    void set_fatal(const std::string& msg) {
        set_error(msg);
        fatal_ = true;
    }

    std::string_view substr_from(size_t start) const {
        return input_.substr(start, pos_ - start);
    }

    // -- Opening hours --
    std::optional<std::vector<RuleSequence>> parse_opening_hours() {
        auto first = parse_rule_sequence(RuleOperator::Normal);
        if (!first) { set_error("expected rule sequence"); return std::nullopt; }

        std::vector<RuleSequence> rules;
        rules.push_back(std::move(*first));

        while (true) {
            auto sp = save();
            auto sep_op = try_parse_rule_separator();
            if (!sep_op) break;
            auto rs = parse_rule_sequence(*sep_op);
            if (!rs) {
                // A trailing separator is OK in relaxed mode
                restore(sp);
                break;
            }
            rules.push_back(std::move(*rs));
        }
        return rules;
    }

    // -- Rule separator --
    std::optional<RuleOperator> try_parse_rule_separator() {
        auto sp = save();
        skip_spaces();
        if (match_str("||")) {
            match_space();
            return RuleOperator::Fallback;
        }
        restore(sp);

        // Normal: optional_space ";" optional_space
        sp = save();
        skip_spaces();
        if (match_char(';')) {
            skip_spaces();
            return RuleOperator::Normal;
        }
        restore(sp);

        // Additional: "," space
        sp = save();
        if (match_char(',') && match_space()) {
            return RuleOperator::Additional;
        }
        restore(sp);

        return std::nullopt;
    }

    // -- Rule sequence --
    std::optional<RuleSequence> parse_rule_sequence(RuleOperator op) {
        auto sp = save();

        // Negative lookahead: not (optional_space (separator | end))
        {
            auto sp2 = save();
            skip_spaces();
            bool is_sep_or_end = at_end()
                || peek() == ';'
                || (peek() == ',' && peek(1) == ' ')
                || (peek() == '|' && peek(1) == '|');
            restore(sp2);
            if (is_sep_or_end) { restore(sp); return std::nullopt; }
        }

        auto sel = parse_selector_sequence();
        if (!sel) { restore(sp); return std::nullopt; }

        auto& [day_sel, time_sel, extra_comment] = *sel;
        skip_spaces();

        RuleKind kind = RuleKind::Open;
        std::optional<std::string> comment;
        if (auto mod = try_parse_rules_modifier()) {
            kind = mod->first;
            comment = std::move(mod->second);
        }

        std::vector<std::string> comments;
        if (comment) comments.push_back(std::move(*comment));
        if (extra_comment) comments.push_back(std::move(*extra_comment));

        return RuleSequence{
            std::move(day_sel), std::move(time_sel),
            kind, op, std::move(comments)
        };
    }

    // -- Rules modifier --
    std::optional<std::pair<RuleKind, std::optional<std::string>>> try_parse_rules_modifier() {
        // Try comment alone
        auto sp = save();
        if (auto c = try_parse_comment()) {
            return std::pair{RuleKind::Open, std::optional<std::string>{std::move(*c)}};
        }
        restore(sp);

        // Try modifier enum
        auto kind = try_parse_rule_kind();
        if (!kind) return std::nullopt;

        skip_spaces();
        auto c = try_parse_comment();
        return std::pair{*kind, c};
    }

    std::optional<RuleKind> try_parse_rule_kind() {
        if (match_str("closed")) return RuleKind::Closed;
        if (match_str("off"))    return RuleKind::Closed;
        if (match_str("open"))   return RuleKind::Open;
        if (match_str("unknown"))return RuleKind::Unknown;
        return std::nullopt;
    }

    // -- Selector sequence --
    using SelectorResult = std::tuple<DaySelector, TimeSelector, std::optional<std::string>>;

    std::optional<SelectorResult> parse_selector_sequence() {
        // Try 24/7
        if (match_str("24/7")) {
            return SelectorResult{DaySelector{}, TimeSelector{}, std::nullopt};
        }

        // wide_range_selectors ~ small_range_selectors?
        auto wide = parse_wide_range_selectors();
        // wide always succeeds (all parts optional in the third alternative)

        if (wide && wide->comment && wide->year.empty() && wide->monthday.empty() &&
            wide->week.empty()) {
            match_str("24/7");
        }

        auto small = try_parse_small_range_selectors();

        DaySelector ds;
        if (wide) {
            ds.year = std::move(wide->year);
            ds.monthday = std::move(wide->monthday);
            ds.week = std::move(wide->week);
        }

        TimeSelector ts;
        if (small) {
            ds.weekday = std::move(small->weekday);
            ts = TimeSelector(std::move(small->time));
        }

        std::optional<std::string> comment;
        if (wide) comment = std::move(wide->comment);

        return SelectorResult{std::move(ds), std::move(ts), std::move(comment)};
    }

    // -- Wide range selectors --
    struct WideResult {
        std::vector<YearRange> year;
        std::vector<MonthdayRange> monthday;
        std::vector<WeekRange> week;
        std::optional<std::string> comment;
    };

    std::optional<WideResult> parse_wide_range_selectors() {
        WideResult wr;

        // Alt 1: comment ":" " "?
        {
            auto sp = save();
            if (auto c = try_parse_comment()) {
                if (match_char(':')) {
                    match_char(' ');
                    wr.comment = std::move(*c);
                    return wr;
                }
            }
            restore(sp);
        }

        // Alt 2: monthday_selector week_selector? separator_for_readability?
        {
            auto sp = save();
            if (auto md = try_parse_monthday_selector()) {
                wr.monthday = std::move(*md);
                if (auto wk = try_parse_week_selector()) wr.week = std::move(*wk);
                try_parse_separator_for_readability();
                return wr;
            }
            restore(sp);
        }

        // Alt 3: year_selector? monthday_selector? week_selector? separator_for_readability?
        {
            if (auto yr = try_parse_year_selector()) wr.year = std::move(*yr);

            // The grammar note says: if a single year could be a year_selector OR
            // monthday prefix, favor monthday.
            if (auto md = try_parse_monthday_selector()) wr.monthday = std::move(*md);
            if (auto wk = try_parse_week_selector()) wr.week = std::move(*wk);
            try_parse_separator_for_readability();
        }

        return wr;
    }

    void try_parse_separator_for_readability() {
        auto sp = save();
        if (match_str(": ")) return;
        restore(sp);
        if (match_char(':')) return;
        match_char(' ');
    }

    // -- Small range selectors --
    struct SmallResult {
        std::vector<WeekDayRange> weekday;
        std::vector<TimeSpan> time;
    };

    std::optional<SmallResult> try_parse_small_range_selectors() {
        SmallResult sr;

        // Alt 1: weekday_selector space time_selector
        {
            auto sp = save();
            if (auto wd = try_parse_weekday_selector()) {
                auto sp2 = save();
                if (match_space()) {
                    if (auto ts = try_parse_time_selector()) {
                        sr.weekday = std::move(*wd);
                        sr.time = std::move(*ts);
                        return sr;
                    }
                }
                restore(sp2);
                // Alt 2: weekday_selector alone
                sr.weekday = std::move(*wd);
                return sr;
            }
            restore(sp);
        }

        // Alt 3: time_selector alone
        if (auto ts = try_parse_time_selector()) {
            sr.time = std::move(*ts);
            return sr;
        }

        return std::nullopt;
    }

    // -- Time selector --
    std::optional<std::vector<TimeSpan>> try_parse_time_selector() {
        auto first = try_parse_timespan();
        if (!first) return std::nullopt;

        std::vector<TimeSpan> spans;
        spans.push_back(std::move(*first));

        while (true) {
            auto sp = save();
            if (match_char(',')) {
                // A comma inside a time block wins over the additive-rule
                // separator, even with a space, as long as a timespan follows
                // (opening_hours.js / opening-hours-rs gh88). A day/month
                // selector after the comma still falls through to an additive
                // rule via the failed timespan parse below.
                skip_spaces();
                if (auto ts = try_parse_timespan()) {
                    spans.push_back(std::move(*ts));
                    continue;
                }
            }
            restore(sp);
            break;
        }
        return spans;
    }

    std::optional<TimeSpan> try_parse_timespan() {
        auto sp = save();
        auto start = try_parse_time();
        if (!start) { restore(sp); return std::nullopt; }

        TimeSpan ts;
        ts.start = *start;

        // Try open-end: time "+"
        {
            auto sp2 = save();
            if (match_char('+')) {
                ts.end = Time::make_fixed(ExtendedTime::midnight_24());
                ts.open_end = true;
                return ts;
            }
            restore(sp2);
        }

        // time space? "-" space? extended_time ...
        {
            auto sp2 = save();
            skip_spaces();
            if (match_char('-')) {
                skip_spaces();
                auto end = try_parse_extended_time();
                if (!end) { restore(sp); return std::nullopt; }
                ts.end = *end;

                // Check for "+", "/" repeat
                if (match_char('+')) {
                    ts.open_end = true;
                } else {
                    auto sp3 = save();
                    skip_spaces();
                    if (match_char('/')) {
                        skip_spaces();
                        // Try hour_minutes as duration
                        if (auto hm = try_parse_hour_minutes()) {
                            ts.repeats_minutes = static_cast<int16_t>(hm->hour * 60 + hm->minute);
                        } else if (auto m = try_parse_minute_value()) {
                            ts.repeats_minutes = *m;
                        } else {
                            restore(sp3);
                        }
                    } else {
                        restore(sp3);
                    }
                }
                return ts;
            }
            restore(sp2);
        }

        // Just a point in time - unsupported, restore
        restore(sp);
        return std::nullopt;
    }

    std::optional<Time> try_parse_time() {
        if (auto hm = try_parse_hour_minutes()) return Time::make_fixed(*hm);
        if (auto vt = try_parse_variable_time()) return Time::make_variable(*vt);
        return std::nullopt;
    }

    std::optional<Time> try_parse_extended_time() {
        if (auto hm = try_parse_extended_hour_minutes()) return Time::make_fixed(*hm);
        if (auto vt = try_parse_variable_time()) return Time::make_variable(*vt);
        return std::nullopt;
    }

    std::optional<VariableTime> try_parse_variable_time() {
        auto sp = save();

        // "(event +/- hour_minutes)"
        if (match_char('(')) {
            auto ev = try_parse_event();
            if (!ev) { restore(sp); return std::nullopt; }
            auto sign = try_parse_plus_or_minus();
            if (!sign) { restore(sp); return std::nullopt; }
            auto hm = try_parse_hour_minutes();
            if (!hm) { restore(sp); return std::nullopt; }
            if (!match_char(')')) { restore(sp); return std::nullopt; }
            int16_t mins = static_cast<int16_t>(hm->mins_from_midnight());
            if (*sign < 0) mins = -mins;
            return VariableTime{*ev, mins};
        }

        // event alone
        if (auto ev = try_parse_event()) {
            return VariableTime{*ev, 0};
        }

        return std::nullopt;
    }

    std::optional<TimeEvent> try_parse_event() {
        if (match_str("dawn"))    return TimeEvent::Dawn;
        if (match_str("sunrise")) return TimeEvent::Sunrise;
        if (match_str("sunset"))  return TimeEvent::Sunset;
        if (match_str("dusk"))    return TimeEvent::Dusk;
        return std::nullopt;
    }

    // Returns +1 or -1
    std::optional<int> try_parse_plus_or_minus() {
        if (match_char('+')) return 1;
        if (match_char('-')) return -1;
        return std::nullopt;
    }

    // -- Time parsing helpers --

    // hour: 00-23 or single digit (relaxed)
    std::optional<uint8_t> try_parse_hour() {
        auto sp = save();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) return std::nullopt;
        uint8_t d1 = peek() - '0'; advance();
        if (std::isdigit(static_cast<unsigned char>(peek()))) {
            uint8_t d2 = peek() - '0'; advance();
            uint8_t h = d1 * 10 + d2;
            if (h <= 23) return h;
            restore(sp); return std::nullopt;
        }
        // single digit
        return d1;
    }

    // extended_hour: 00-48 or single digit (relaxed)
    std::optional<uint8_t> try_parse_extended_hour() {
        auto sp = save();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) return std::nullopt;
        uint8_t d1 = peek() - '0'; advance();
        if (std::isdigit(static_cast<unsigned char>(peek()))) {
            uint8_t d2 = peek() - '0'; advance();
            uint8_t h = d1 * 10 + d2;
            if (h <= 48) return h;
            restore(sp); return std::nullopt;
        }
        return d1;
    }

    std::optional<uint8_t> try_parse_minute() {
        if (!std::isdigit(static_cast<unsigned char>(peek()))) return std::nullopt;
        if (!std::isdigit(static_cast<unsigned char>(peek(1)))) { return std::nullopt; }
        uint8_t m = (peek() - '0') * 10 + (peek(1) - '0');
        if (m > 59) { return std::nullopt; }
        advance(2);
        return m;
    }

    std::optional<int16_t> try_parse_minute_value() {
        auto m = try_parse_minute();
        if (!m) return std::nullopt;
        return static_cast<int16_t>(*m);
    }

    // hour_minutes: HH:MM or "24:00"
    std::optional<ExtendedTime> try_parse_hour_minutes() {
        auto sp = save();
        // Try "24:00" first
        if (match_str("24:00")) return ExtendedTime::midnight_24();

        auto h = try_parse_hour();
        if (!h) { restore(sp); return std::nullopt; }
        if (!match_char(':')) { restore(sp); return std::nullopt; }
        auto m = try_parse_minute();
        if (!m) { restore(sp); return std::nullopt; }
        auto t = ExtendedTime::make(*h, *m);
        if (!t) { restore(sp); return std::nullopt; }
        return t;
    }

    // extended_hour_minutes: HH:MM where H can go up to 48
    std::optional<ExtendedTime> try_parse_extended_hour_minutes() {
        auto sp = save();
        auto h = try_parse_extended_hour();
        if (!h) { restore(sp); return std::nullopt; }
        if (!match_char(':')) { restore(sp); return std::nullopt; }
        auto m = try_parse_minute();
        if (!m) { restore(sp); return std::nullopt; }
        auto t = ExtendedTime::make(*h, *m);
        if (!t) { restore(sp); return std::nullopt; }
        return t;
    }

    // -- Weekday selector --
    std::optional<std::vector<WeekDayRange>> try_parse_weekday_selector() {
        // Try holiday_sequence first, then optional weekday_sequence
        {
            auto sp = save();
            if (auto hol = try_parse_holiday_sequence()) {
                std::vector<WeekDayRange> result = std::move(*hol);
                auto sp2 = save();
                if ((match_char(',') || match_space()) && !at_end()) {
                    if (auto wd = try_parse_weekday_sequence()) {
                        result.insert(result.end(), wd->begin(), wd->end());
                        return result;
                    }
                }
                restore(sp2);
                return result;
            }
            restore(sp);
        }

        // Try weekday_sequence, then optional holiday_sequence
        {
            auto sp = save();
            if (auto wd = try_parse_weekday_sequence()) {
                std::vector<WeekDayRange> result = std::move(*wd);
                auto sp2 = save();
                if ((match_char(',') || match_space()) && !at_end()) {
                    if (auto hol = try_parse_holiday_sequence()) {
                        result.insert(result.end(), hol->begin(), hol->end());
                        return result;
                    }
                }
                restore(sp2);
                return result;
            }
            restore(sp);
        }

        return std::nullopt;
    }

    std::optional<std::vector<WeekDayRange>> try_parse_weekday_sequence() {
        auto first = try_parse_weekday_range();
        if (!first) return std::nullopt;
        std::vector<WeekDayRange> result;
        result.push_back(std::move(*first));
        while (true) {
            auto sp = save();
            if (match_char(',')) {
                skip_spaces();  // allow "Su, Sa" list, not just "Su,Sa"
                if (auto wr = try_parse_weekday_range()) {
                    result.push_back(std::move(*wr));
                    continue;
                }
            }
            restore(sp);
            break;
        }
        return result;
    }

    std::optional<WeekDayRange> try_parse_weekday_range() {
        auto sp = save();
        auto start = try_parse_wday();
        if (!start) { restore(sp); return std::nullopt; }

        WeekDayRange wr = WeekDayRange::make_fixed(*start, *start);

        // Check for range: wday "-" wday
        {
            auto sp2 = save();
            if (match_char('-')) {
                if (auto end = try_parse_wday()) {
                    wr.range_end = *end;
                } else {
                    restore(sp2);
                }
            } else {
                restore(sp2);
            }
        }

        // Check for nth entries: "[" nth_entry ("," nth_entry)* "]"
        if (peek() == '[') {
            advance();
            // Reset nth arrays to all-false
            std::fill(std::begin(wr.nth_from_start), std::end(wr.nth_from_start), false);
            std::fill(std::begin(wr.nth_from_end), std::end(wr.nth_from_end), false);

            // Parse nth entries
            auto parse_one_nth_entry = [&]() -> bool {
                bool negative = false;
                if (match_char('-')) negative = true;
                auto n = try_parse_nth();
                if (!n) return false;
                uint8_t start_n = *n;
                uint8_t end_n = start_n;

                // Check for range
                auto sp3 = save();
                if (match_char('-')) {
                    if (auto n2 = try_parse_nth()) {
                        end_n = *n2;
                    } else {
                        restore(sp3);
                    }
                } else {
                    restore(sp3);
                }

                for (uint8_t k = start_n; k <= end_n; ++k) {
                    if (negative) wr.nth_from_end[k - 1] = true;
                    else wr.nth_from_start[k - 1] = true;
                }
                return true;
            };

            if (!parse_one_nth_entry()) { restore(sp); return std::nullopt; }
            while (match_char(',')) {
                if (!parse_one_nth_entry()) break;
            }
            if (!match_char(']')) { restore(sp); return std::nullopt; }
        }

        // Check for day_offset
        if (auto off = try_parse_day_offset()) {
            wr.offset = *off;
        }

        return wr;
    }

    std::optional<uint8_t> try_parse_nth() {
        if (peek() >= '1' && peek() <= '5') {
            uint8_t n = peek() - '0'; advance(); return n;
        }
        return std::nullopt;
    }

    std::optional<Weekday> try_parse_wday() {
        if (match_str("Mo")) return Weekday::Mon;
        if (match_str("Tu")) return Weekday::Tue;
        if (match_str("We")) return Weekday::Wed;
        if (match_str("Th")) return Weekday::Thu;
        if (match_str("Fr")) return Weekday::Fri;
        if (match_str("Sa")) return Weekday::Sat;
        if (match_str("Su")) return Weekday::Sun;
        return std::nullopt;
    }

    // -- Holiday --
    std::optional<std::vector<WeekDayRange>> try_parse_holiday_sequence() {
        auto first = try_parse_holiday();
        if (!first) return std::nullopt;
        std::vector<WeekDayRange> result;
        result.push_back(std::move(*first));
        while (true) {
            auto sp = save();
            if (match_char(',')) {
                skip_spaces();  // allow "PH, SH" list, not just "PH,SH"
                if (auto h = try_parse_holiday()) {
                    result.push_back(std::move(*h));
                    continue;
                }
            }
            restore(sp);
            break;
        }
        return result;
    }

    std::optional<WeekDayRange> try_parse_holiday() {
        HolidayKind hk;
        if (match_str("PH")) hk = HolidayKind::Public;
        else if (match_str("SH")) hk = HolidayKind::School;
        else return std::nullopt;

        int64_t off = 0;
        if (auto o = try_parse_day_offset()) off = *o;
        return WeekDayRange::make_holiday(hk, off);
    }

    std::optional<int64_t> try_parse_day_offset() {
        auto sp = save();
        if (!match_space()) return std::nullopt;
        auto sign = try_parse_plus_or_minus();
        if (!sign) { restore(sp); return std::nullopt; }
        auto num = try_parse_positive_number();
        if (!num) { restore(sp); return std::nullopt; }
        if (!match_space()) { restore(sp); return std::nullopt; }
        if (!match_str("day")) { restore(sp); return std::nullopt; }
        match_char('s'); // optional plural
        int64_t val = static_cast<int64_t>(*num);
        return (*sign < 0) ? -val : val;
    }

    // -- Week selector --
    std::optional<std::vector<WeekRange>> try_parse_week_selector() {
        auto sp = save();
        // Optional separator_for_readability before "week"
        try_parse_separator_for_readability();
        if (!match_str("week")) { restore(sp); return std::nullopt; }
        skip_spaces();

        auto first = try_parse_week();
        if (!first) { restore(sp); return std::nullopt; }
        std::vector<WeekRange> result;
        result.push_back(*first);
        while (true) {
            auto sp2 = save();
            if (match_char(',')) {
                skip_spaces();  // allow "week 1, 3" list
                if (auto w = try_parse_week()) {
                    result.push_back(*w);
                    continue;
                }
            }
            restore(sp2);
            break;
        }
        return result;
    }

    std::optional<WeekRange> try_parse_week() {
        auto start = try_parse_weeknum();
        if (!start) return std::nullopt;
        WeekRange wr;
        wr.start = *start;
        wr.end = *start;

        auto sp = save();
        if (match_char('-')) {
            if (auto end = try_parse_weeknum()) {
                wr.end = *end;
                // Reject inverted ranges like the Rust lib (InvertedWeekRange).
                if (wr.end < wr.start)
                    set_fatal("inverted week range " + fmt02(wr.start) + "-" + fmt02(wr.end));
                auto sp2 = save();
                if (match_char('/')) {
                    if (auto step = try_parse_positive_number()) {
                        wr.step = static_cast<uint8_t>(*step);
                        // Clamp step to 1 when it exceeds the range span, matching
                        // Rust's `WeekRange::new`.
                        if (wr.end - wr.start < wr.step) wr.step = 1;
                    } else {
                        restore(sp2);
                    }
                } else {
                    restore(sp2);
                }
            } else {
                restore(sp);
            }
        } else {
            restore(sp);
        }
        return wr;
    }

    std::optional<uint8_t> try_parse_weeknum() {
        auto sp = save();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) return std::nullopt;
        uint8_t d1 = peek() - '0'; advance();
        if (std::isdigit(static_cast<unsigned char>(peek()))) {
            uint8_t d2 = peek() - '0'; advance();
            uint8_t n = d1 * 10 + d2;
            if (n >= 1 && n <= 53) return n;
            restore(sp); return std::nullopt;
        }
        // single digit (relaxed): 1-9
        if (d1 >= 1 && d1 <= 9) return d1;
        restore(sp); return std::nullopt;
    }

    // -- Monthday selector --
    std::optional<std::vector<MonthdayRange>> try_parse_monthday_selector() {
        auto first = try_parse_monthday_range();
        if (!first) return std::nullopt;
        std::vector<MonthdayRange> result;
        result.push_back(std::move(*first));
        while (true) {
            auto sp = save();
            if (match_char(',')) {
                skip_spaces();  // allow "Jan, Mar" list, not just "Jan,Mar"
                if (auto mr = try_parse_monthday_range()) {
                    result.push_back(std::move(*mr));
                    continue;
                }
            }
            restore(sp);
            break;
        }
        return result;
    }

    std::optional<MonthdayRange> try_parse_monthday_range() {
        // Alt 1-3: date_from based
        {
            auto sp = save();

            // Check for optional year prefix for date_from
            std::optional<uint16_t> year_prefix;
            auto sp_y = save();
            if (auto y = try_parse_year()) {
                year_prefix = *y;
                match_char(' '); // optional space after year
            } else {
                restore(sp_y);
            }

            // Try month + optional day → date_from or month range
            if (auto month = try_parse_month()) {
                // Could be: month "-" month  (month range)
                // Or: month " " daynum  (date)
                // Or: month " " wday    (weekday-in-month)
                // Or: just month (month range with same start/end)

                // Try month "-" month first
                {
                    auto sp2 = save();
                    if (match_char('-')) {
                        if (auto m2 = try_parse_month()) {
                            return MonthdayRange::make_month(year_prefix, *month, *m2);
                        }
                    }
                    restore(sp2);
                }

                // Try month " "? (daynum | wday) for date_from
                {
                    auto sp2 = save();
                    match_char(' ');

                    // Try daynum
                    if (auto day = try_parse_daynum()) {
                        Date date = Date::make_fixed(year_prefix, *month, *day);
                        return finish_monthday_range_from_date(date);
                    }

                    // Try weekday-in-month. A bare weekday belongs to the
                    // small selector after the month selector (for example
                    // "Apr Mo,Th"), so require the Rust grammar's [n] suffix.
                    if (auto wd = try_parse_wday()) {
                        if (match_char('[')) {
                            bool neg = false;
                            if (match_char('-')) neg = true;
                            auto n = try_parse_nth();
                            if (n && match_char(']')) {
                                int8_t nth = neg ? -static_cast<int8_t>(*n) : static_cast<int8_t>(*n);
                                Date date = Date::make_weekday_in_month(year_prefix, *month, *wd, nth);
                                return finish_monthday_range_from_date(date);
                            }
                        }
                    }

                    restore(sp2);
                }

                // Just a month (or year + month)
                return MonthdayRange::make_month(year_prefix, *month, *month);
            }

            // Try variable_date (easter)
            if (match_str_ascii_ci("easter")) {
                Date date = Date::make_easter(year_prefix);
                return finish_monthday_range_from_date(date);
            }

            restore(sp);
        }

        return std::nullopt;
    }

    std::optional<MonthdayRange> finish_monthday_range_from_date(Date start_date) {
        DateOffset start_offset;
        if (auto off = try_parse_date_offset()) start_offset = *off;

        // Check for "-" date_to
        {
            auto sp = save();
            skip_spaces();
            if (match_char('-')) {
                skip_spaces();
                if (auto end = try_parse_date_to(start_date)) {
                    DateOffset end_offset;
                    if (auto off = try_parse_date_offset()) end_offset = *off;
                    return MonthdayRange::make_date(start_date, start_offset, *end, end_offset);
                }
            }
            restore(sp);
        }

        // Check for "+"
        {
            auto sp = save();
            if (match_char('+')) {
                Date end_date = start_date.has_year()
                    ? Date::make_fixed(9999, Month::Dec, 31)
                    : Date::make_fixed(std::nullopt, Month::Dec, 31);
                return MonthdayRange::make_date(start_date, start_offset, end_date, {});
            }
            restore(sp);
        }

        // Single date
        return MonthdayRange::make_date(start_date, start_offset, start_date, start_offset);
    }

    std::optional<Date> try_parse_date_to(const Date& from) {
        // Try full date_from
        {
            auto sp = save();
            std::optional<uint16_t> year_prefix;
            auto sp_y = save();
            if (auto y = try_parse_year()) {
                year_prefix = *y;
                match_char(' ');
            } else {
                restore(sp_y);
            }

            if (auto month = try_parse_month()) {
                match_char(' ');
                if (auto day = try_parse_daynum()) {
                    return Date::make_fixed(year_prefix, *month, *day);
                }
                if (auto wd = try_parse_wday()) {
                    if (match_char('[')) {
                        bool neg = false;
                        if (match_char('-')) neg = true;
                        auto n = try_parse_nth();
                        if (n && match_char(']')) {
                            int8_t nth = neg ? -static_cast<int8_t>(*n) : static_cast<int8_t>(*n);
                            return Date::make_weekday_in_month(year_prefix, *month, *wd, nth);
                        }
                    }
                }
                restore(sp);
            } else {
                if (match_str_ascii_ci("easter")) {
                    return Date::make_easter(year_prefix);
                }
                restore(sp);
            }

            if (match_str_ascii_ci("easter")) {
                return Date::make_easter(year_prefix);
            }
        }

        // Try bare daynum: the end inherits month/year from the start date
        // (mirrors Rust `build_date_to`).
        if (auto day = try_parse_daynum()) {
            if (from.kind == DateKind::Fixed) {
                auto month = from.month;
                auto year = from.year;
                if (from.day > *day) {
                    month = next_month(month);
                    if (month == Month::Jan && year) *year += 1;
                }
                return Date::make_fixed(year, month, *day);
            }
            if (from.kind == DateKind::WeekdayInMonth) {
                // e.g. "Jan Mo[1]-15" -> end is Jan 15.
                return Date::make_fixed(from.year, from.month, *day);
            }
            // Easter followed by a day number is unsupported (ambiguous).
        }
        return std::nullopt;
    }

    std::optional<DateOffset> try_parse_date_offset() {
        auto sp_outer = save();
        DateOffset off;

        // Try +/- wday
        {
            auto sp2 = save();
            auto sign = try_parse_plus_or_minus();
            if (sign) {
                if (auto wd = try_parse_wday()) {
                    off.wday_kind = (*sign > 0) ? WeekDayOffsetKind::Next : WeekDayOffsetKind::Prev;
                    off.wday = *wd;
                } else {
                    restore(sp2);
                }
            } else {
                restore(sp2);
            }
        }

        // Try day_offset
        if (auto doff = try_parse_day_offset()) {
            off.day_offset = *doff;
        }

        if (off.wday_kind == WeekDayOffsetKind::None && off.day_offset == 0) {
            restore(sp_outer);
            return std::nullopt;
        }
        return off;
    }

    std::optional<uint8_t> try_parse_daynum() {
        auto sp = save();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) return std::nullopt;

        uint8_t d1 = peek() - '0'; advance();
        uint8_t num = d1;

        if (std::isdigit(static_cast<unsigned char>(peek()))) {
            uint8_t d2 = peek() - '0'; advance();
            num = d1 * 10 + d2;
        }

        if (num < 1 || num > 31) { restore(sp); return std::nullopt; }

        // Negative lookahead: not followed by ":" minute (time pattern)
        // unless that minute is also followed by ":" minute
        {
            auto sp2 = save();
            if (peek() == ':') {
                advance();
                if (std::isdigit(static_cast<unsigned char>(peek()))
                    && std::isdigit(static_cast<unsigned char>(peek(1)))) {
                    // Check if followed by another ":MM"
                    size_t after_min = pos_ + 2;
                    if (after_min < input_.size() && input_[after_min] == ':'
                        && after_min + 1 < input_.size()
                        && std::isdigit(static_cast<unsigned char>(input_[after_min + 1]))) {
                        // DD:MM:MM pattern → ok, it's a daynum
                        restore(sp2);
                    } else {
                        // DD:MM pattern → not a daynum, it's a time
                        restore(sp);
                        return std::nullopt;
                    }
                } else {
                    restore(sp2);
                }
            }
        }

        return num;
    }

    // -- Year selector --
    std::optional<std::vector<YearRange>> try_parse_year_selector() {
        auto first = try_parse_year_range();
        if (!first) return std::nullopt;
        std::vector<YearRange> result;
        result.push_back(*first);
        while (true) {
            auto sp = save();
            if (match_char(',')) {
                skip_spaces();  // allow "2020, 2022" list
                if (auto yr = try_parse_year_range()) {
                    result.push_back(*yr);
                    continue;
                }
            }
            restore(sp);
            break;
        }
        return result;
    }

    std::optional<YearRange> try_parse_year_range() {
        auto start = try_parse_year();
        if (!start) return std::nullopt;
        YearRange yr;
        yr.start = *start;
        yr.end = *start;

        // Check for "+"
        if (match_char('+')) { yr.end = 9999; return yr; }

        // Check for "-" year
        auto sp = save();
        if (match_char('-')) {
            if (auto end = try_parse_year()) {
                yr.end = *end;
                // Reject inverted ranges like the Rust lib (InvertedYearRange).
                if (yr.end < yr.start)
                    set_fatal("inverted year range " + std::to_string(yr.start) + "-"
                              + std::to_string(yr.end));
                auto sp2 = save();
                if (match_char('/')) {
                    if (auto step = try_parse_positive_number()) {
                        yr.step = static_cast<uint16_t>(*step);
                        // Clamp step to 1 when it exceeds the range span, matching
                        // Rust's `YearRange::new`.
                        if (yr.end - yr.start < yr.step) yr.step = 1;
                    } else {
                        restore(sp2);
                    }
                } else {
                    restore(sp2);
                }
                return yr;
            }
            restore(sp);
        } else {
            restore(sp);
        }
        return yr;
    }

    std::optional<uint16_t> try_parse_year() {
        if (pos_ + 4 > input_.size()) return std::nullopt;

        // Must be 4 digits
        for (int k = 0; k < 4; ++k)
            if (!std::isdigit(static_cast<unsigned char>(input_[pos_ + k])))
                return std::nullopt;

        uint16_t y = 0;
        for (int k = 0; k < 4; ++k)
            y = y * 10 + (input_[pos_ + k] - '0');

        // Valid: 1900-9999
        if (y < 1900) return std::nullopt;
        advance(4);
        return y;
    }

    std::optional<Month> try_parse_month() {
        if (match_str("Jan")) return Month::Jan;
        if (match_str("Feb")) return Month::Feb;
        if (match_str("Mar")) return Month::Mar;
        if (match_str("Apr")) return Month::Apr;
        if (match_str("May")) return Month::May;
        if (match_str("Jun")) return Month::Jun;
        if (match_str("Jul")) return Month::Jul;
        if (match_str("Aug")) return Month::Aug;
        if (match_str("Sep")) return Month::Sep;
        if (match_str("Oct")) return Month::Oct;
        if (match_str("Nov")) return Month::Nov;
        if (match_str("Dec")) return Month::Dec;
        return std::nullopt;
    }

    std::optional<uint64_t> try_parse_positive_number() {
        if (!std::isdigit(static_cast<unsigned char>(peek()))) return std::nullopt;
        uint64_t n = 0;
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            n = n * 10 + (peek() - '0');
            advance();
        }
        if (n == 0) return std::nullopt; // must be positive
        return n;
    }

    // -- Comment --
    std::optional<std::string> try_parse_comment() {
        if (!match_char('"')) return std::nullopt;
        std::string text;
        while (!at_end() && peek() != '"') {
            text += peek();
            advance();
        }
        if (!match_char('"')) return std::nullopt; // unclosed
        return text;
    }
};

} // namespace detail

// ---------------------------------------------------------------------------
// parse_normalized  –  Pass 2
// ---------------------------------------------------------------------------
inline std::expected<OpeningHoursExpression, std::string>
parse_normalized(std::string_view input) {
    detail::Parser parser(input);
    return parser.parse();
}

// ============================================================================
// ============================================================================
// PASS 3 – Dedup / simplification
// ============================================================================
// ============================================================================

namespace detail {

// Merge overlapping/adjacent TimeSpans (only for Fixed times).
inline void merge_timespans(std::vector<TimeSpan>& spans) {
    if (spans.size() <= 1) return;

    // Sort by start time (Fixed times only; Variable times stay in place)
    std::stable_sort(spans.begin(), spans.end(), [](const TimeSpan& a, const TimeSpan& b) {
        if (a.start.tag != Time::Fixed || b.start.tag != Time::Fixed) return false;
        return a.start.fixed < b.start.fixed;
    });

    std::vector<TimeSpan> merged;
    merged.push_back(spans[0]);

    for (size_t i = 1; i < spans.size(); ++i) {
        auto& prev = merged.back();
        auto& cur = spans[i];

        // Only merge Fixed-Fixed spans without open_end or repeats
        if (prev.start.tag == Time::Fixed && prev.end.tag == Time::Fixed
            && cur.start.tag == Time::Fixed && cur.end.tag == Time::Fixed
            && !prev.open_end && !cur.open_end
            && !prev.repeats_minutes && !cur.repeats_minutes)
        {
            // Check if cur starts <= prev ends (overlap or adjacent)
            if (cur.start.fixed <= prev.end.fixed) {
                if (cur.end.fixed > prev.end.fixed) {
                    prev.end.fixed = cur.end.fixed;
                }
                continue; // merged
            }
        }
        merged.push_back(cur);
    }
    spans = std::move(merged);
}

// Deduplicate a sorted vector.
template<typename T>
void dedup_vec(std::vector<T>& v) {
    auto last = std::unique(v.begin(), v.end());
    v.erase(last, v.end());
}

// Sort YearRanges by start year.
inline void sort_dedup_years(std::vector<YearRange>& v) {
    std::stable_sort(v.begin(), v.end(), [](const YearRange& a, const YearRange& b) {
        return a.start < b.start;
    });
    dedup_vec(v);
}

// Sort WeekRanges.
inline void sort_dedup_weeks(std::vector<WeekRange>& v) {
    std::stable_sort(v.begin(), v.end(), [](const WeekRange& a, const WeekRange& b) {
        return a.start < b.start;
    });
    dedup_vec(v);
}

// Sort WeekDayRanges.
inline void sort_dedup_weekdays(std::vector<WeekDayRange>& v) {
    std::stable_sort(v.begin(), v.end(), [](const WeekDayRange& a, const WeekDayRange& b) {
        if (a.kind != b.kind) return a.kind < b.kind;
        if (a.kind == WeekDayRangeKind::FixedDays)
            return static_cast<int>(a.range_start) < static_cast<int>(b.range_start);
        return static_cast<int>(a.holiday_kind) < static_cast<int>(b.holiday_kind);
    });
    dedup_vec(v);
}

} // namespace detail

// ---------------------------------------------------------------------------
// dedup  –  Pass 3
// ---------------------------------------------------------------------------
inline OpeningHoursExpression dedup(OpeningHoursExpression expr) {
    for (auto& rule : expr.rules) {
        // Dedup day selectors
        detail::sort_dedup_years(rule.day_selector.year);
        detail::sort_dedup_weeks(rule.day_selector.week);
        detail::sort_dedup_weekdays(rule.day_selector.weekday);

        // Merge overlapping time spans
        detail::merge_timespans(rule.time_selector.time);

        // Dedup comments
        std::sort(rule.comments.begin(), rule.comments.end());
        detail::dedup_vec(rule.comments);
    }

    // Remove fully duplicate rule sequences
    {
        std::vector<RuleSequence> unique_rules;
        for (auto& rule : expr.rules) {
            bool dup = false;
            for (const auto& existing : unique_rules) {
                if (rule == existing) { dup = true; break; }
            }
            if (!dup) unique_rules.push_back(std::move(rule));
        }
        expr.rules = std::move(unique_rules);
    }

    return expr;
}

// ============================================================================
// ============================================================================
// Top-level API
// ============================================================================
// ============================================================================

/// Full pipeline: normalize → parse → dedup.
inline std::expected<OpeningHoursExpression, std::string>
parse(std::string_view input) {
    std::string normalized = normalize_input(input);
    auto result = parse_normalized(normalized);
    if (!result) return result;
    return dedup(std::move(*result));
}

} // namespace opening_hours
