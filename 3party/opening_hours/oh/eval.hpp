// opening_hours_eval.hpp – C++23 evaluation engine for opening_hours
// ============================================================================
//
// This is the C++ port of the Rust `opening-hours` evaluation crate (the
// `opening-hours-syntax` crate is ported in `opening_hours.hpp`, which this
// header includes). It turns a parsed `OpeningHoursExpression` plus a `Context`
// into:
//
//   * schedule_at(date) -> Schedule
//   * state(dt)         -> (RuleKind, comment)
//   * is_open / is_closed / is_unknown
//   * next_change(dt)   -> optional<DateTime>
//   * iter_range / iter_from  (interval iterator)
//
// Everything lives in `namespace opening_hours`, depends only on the C++23
// standard library, and mirrors the Rust reference 1:1 (file:line references
// are given at the top of each section) so the two stay easy to keep in sync.
//
// Build (see opening_hours_eval_test.cpp for the exact command):
//   clang++ -std=c++23 -O2 -Wall -Wextra opening_hours_eval_test.cpp
//
// ============================================================================
#pragma once

#include "parser.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace opening_hours {

namespace chrono = std::chrono;

// ============================================================================
// Date & time foundation
//
// The Rust engine works over `chrono::NaiveDate` / `NaiveDateTime`. We map
// those onto `std::chrono`'s calendar types (`sys_days`, `year_month_day`, …),
// which give us dependency-free, correct calendar arithmetic.
//
//   NaiveDate      ~ chrono::NaiveDate      (a calendar day)
//   NaiveDateTime  ~ chrono::NaiveDateTime  (a day + minutes-from-midnight)
//
// Weekday numbering follows the AST enum (Mon = 0 … Sun = 6), matching Rust's
// `chrono::Weekday as u8`.
// ============================================================================

/// Floored integer division (matches Rust's `Euclidean`-style day carry).
constexpr int64_t floordiv(int64_t a, int64_t b) {
    int64_t q = a / b;
    int64_t r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) --q;
    return q;
}

// --- NaiveDate ---

struct NaiveDate {
    chrono::sys_days d{}; // days since 1970-01-01

    constexpr NaiveDate() = default;
    constexpr explicit NaiveDate(chrono::sys_days sd) : d(sd) {}

    /// Build a date, returning `nullopt` for out-of-range values (e.g. Feb 30).
    static std::optional<NaiveDate> from_ymd(int year, unsigned month, unsigned day) {
        if (month < 1 || month > 12 || day < 1 || day > 31) return std::nullopt;
        chrono::year_month_day ymd{chrono::year{year}, chrono::month{month}, chrono::day{day}};
        if (!ymd.ok()) return std::nullopt;
        return NaiveDate{chrono::sys_days{ymd}};
    }

    /// Build a date assumed valid (caller guarantees validity).
    static NaiveDate ymd_unchecked(int year, unsigned month, unsigned day) {
        return NaiveDate{chrono::sys_days{chrono::year{year} / chrono::month{month} / chrono::day{day}}};
    }

    chrono::year_month_day ymd() const { return chrono::year_month_day{d}; }
    int year() const { return int(ymd().year()); }
    unsigned month() const { return unsigned(ymd().month()); }
    unsigned day() const { return unsigned(ymd().day()); }

    /// Weekday as Mon = 0 … Sun = 6.
    int weekday_index() const { return int(chrono::weekday{d}.iso_encoding()) - 1; }
    Weekday weekday() const { return static_cast<Weekday>(weekday_index()); }

    NaiveDate add_days(int64_t n) const { return NaiveDate{d + chrono::days{n}}; }
    NaiveDate succ() const { return add_days(1); }
    NaiveDate pred() const { return add_days(-1); }

    int64_t day_number() const { return d.time_since_epoch().count(); }

    auto operator<=>(const NaiveDate&) const = default;
    bool operator==(const NaiveDate&) const = default;
};

/// ISO-8601 week-year and week number.
struct IsoWeek {
    int year;
    int week;
    auto operator<=>(const IsoWeek&) const = default;
};

inline IsoWeek iso_week(NaiveDate date) {
    // The ISO week of a date is the week that contains its Thursday.
    int wd = int(chrono::weekday{date.d}.iso_encoding()); // 1..7 (Mon..Sun)
    chrono::sys_days thursday = date.d + chrono::days{4 - wd};
    int iso_year = int(chrono::year_month_day{thursday}.year());
    chrono::sys_days jan1 = chrono::sys_days{chrono::year{iso_year} / chrono::January / chrono::day{1}};
    int week = int((thursday - jan1).count() / 7) + 1;
    return {iso_year, week};
}

/// Build a date from an ISO year + week + weekday (Mon..Sun), `nullopt` if the
/// week does not exist in that year (mirrors `chrono::from_isoywd_opt`).
inline std::optional<NaiveDate> from_isoywd(int iso_year, int week, Weekday wd) {
    if (week < 1 || week > 53) return std::nullopt;
    chrono::sys_days jan4 = chrono::sys_days{chrono::year{iso_year} / chrono::January / chrono::day{4}};
    int jan4_wd = int(chrono::weekday{jan4}.iso_encoding()); // 1..7
    chrono::sys_days week1_monday = jan4 - chrono::days{jan4_wd - 1};
    int wd_iso = static_cast<int>(wd) + 1; // Mon=1 .. Sun=7
    NaiveDate result{week1_monday + chrono::days{(week - 1) * 7 + (wd_iso - 1)}};
    // Reject weeks that overflow into a neighbouring ISO year.
    IsoWeek check = iso_week(result);
    if (check.year != iso_year || check.week != week) return std::nullopt;
    return result;
}

/// Build the `nth` (1-based) `wd` of the given month, `nullopt` if it does not
/// exist (mirrors `chrono::from_weekday_of_month_opt`).
inline std::optional<NaiveDate> from_weekday_of_month(int y, unsigned m, Weekday wd, int nth) {
    if (nth < 1 || nth > 5) return std::nullopt;
    chrono::weekday cw{static_cast<unsigned>(static_cast<int>(wd) + 1)}; // Mon(0)->1 … Sun(6)->7
    chrono::year_month_weekday ymw{chrono::year{y}, chrono::month{m},
                                   chrono::weekday_indexed{cw, static_cast<unsigned>(nth)}};
    chrono::sys_days sd{ymw};
    chrono::year_month_day back{sd};
    if (unsigned(back.month()) != m || int(back.year()) != y) return std::nullopt;
    return NaiveDate{sd};
}

/// Number of days in the month containing `date`.
inline unsigned count_days_in_month(NaiveDate date) {
    auto ymd = date.ymd();
    chrono::year_month_day_last last{ymd.year(), chrono::month_day_last{ymd.month()}};
    return unsigned(last.day());
}

/// Easter date for a given year (Anonymous Gregorian algorithm).
inline std::optional<NaiveDate> easter(int year) {
    long a = year % 19;
    long b = year / 100;
    long c = year % 100;
    long d = b / 4;
    long e = b % 4;
    long f = (b + 8) / 25;
    long g = (b - f + 1) / 3;
    long h = (19 * a + b - d - g + 15) % 30;
    long i = c / 4;
    long k = c % 4;
    long l = (32 + 2 * e + 2 * i - h - k) % 7;
    long m = (a + 11 * h + 22 * l) / 451;
    long n = (h + l - 7 * m + 114) / 31;
    long o = (h + l - 7 * m + 114) % 31;
    return NaiveDate::from_ymd(year, unsigned(n), unsigned(o + 1));
}

/// Apply an AST `DateOffset` (day shift + optional weekday snap) to a date.
inline NaiveDate apply_date_offset(const DateOffset& off, NaiveDate date) {
    date = date.add_days(off.day_offset);
    switch (off.wday_kind) {
    case WeekDayOffsetKind::None:
        break;
    case WeekDayOffsetKind::Prev: {
        int diff = (7 + date.weekday_index() - static_cast<int>(off.wday)) % 7;
        date = date.add_days(-diff);
        break;
    }
    case WeekDayOffsetKind::Next: {
        int diff = (7 + static_cast<int>(off.wday) - date.weekday_index()) % 7;
        date = date.add_days(diff);
        break;
    }
    }
    return date;
}

// --- NaiveDateTime ---

/// A naive (timezone-agnostic) date-time: a calendar day plus minutes from
/// midnight. Times are always in [00:00, 24:00) — extended times only appear
/// while building day schedules, never in a materialised datetime.
struct NaiveDateTime {
    chrono::sys_days date_{}; // the day
    int minute_ = 0;          // minutes from midnight, [0, 1440)

    constexpr NaiveDateTime() = default;

    NaiveDateTime(NaiveDate d, int minute_of_day) {
        *this = normalized(d.d, minute_of_day);
    }
    NaiveDateTime(chrono::sys_days d, int minute_of_day) {
        *this = normalized(d, minute_of_day);
    }

    /// Build a normalized datetime, carrying minutes over/under a day.
    static NaiveDateTime normalized(chrono::sys_days d, int64_t minute) {
        int64_t day_delta = floordiv(minute, 1440);
        int64_t nm = minute - day_delta * 1440;
        NaiveDateTime r;
        r.date_ = d + chrono::days{day_delta};
        r.minute_ = int(nm);
        return r;
    }

    /// Build a datetime from a date and an `ExtendedTime` (0..24h in practice).
    static NaiveDateTime from_date_time(NaiveDate d, ExtendedTime t) {
        return normalized(d.d, int64_t(t.hour) * 60 + t.minute);
    }

    NaiveDate date() const { return NaiveDate{date_}; }
    int minute_of_day() const { return minute_; }
    int hour() const { return minute_ / 60; }
    int minute_of_hour() const { return minute_ % 60; }

    NaiveDateTime add_minutes(int64_t m) const {
        return normalized(date_, int64_t(minute_) + m);
    }

    auto operator<=>(const NaiveDateTime&) const = default;
    bool operator==(const NaiveDateTime&) const = default;
};

// --- Global evaluation bounds (opening_hours.rs:22-33) ---

inline const NaiveDate DATE_START_DATE = NaiveDate::ymd_unchecked(1900, 1, 1);
inline const NaiveDate DATE_END_DATE = NaiveDate::ymd_unchecked(10000, 1, 1);
inline const NaiveDateTime DATE_START{DATE_START_DATE, 0};
inline const NaiveDateTime DATE_END{DATE_END_DATE, 0};

// ============================================================================
// CompactCalendar  (compact-calendar/src/lib.rs)
//
// A bit-array set of calendar days, used to store holiday dates. Ported as a
// pure data structure (contains / first_after / insert / count); the blob
// (de)serialization is intentionally skipped — consumers insert their own
// holiday dates.
// ============================================================================

struct CompactMonth {
    uint32_t bits = 0; // bit (day-1) set == day included

    bool insert(unsigned day) {
        uint32_t mask = 1u << (day - 1);
        if (bits & mask) return false;
        bits |= mask;
        return true;
    }
    bool contains(unsigned day) const { return (bits & (1u << (day - 1))) != 0; }
    std::optional<unsigned> first() const {
        if (bits == 0) return std::nullopt;
        return unsigned(std::countr_zero(bits)) + 1;
    }
    std::optional<unsigned> first_after(unsigned day) const {
        uint32_t shifted = (day >= 32) ? 0u : (bits >> day);
        if (shifted == 0) return std::nullopt;
        return day + unsigned(std::countr_zero(shifted)) + 1;
    }
    unsigned count() const { return unsigned(std::popcount(bits)); }
};

struct CompactYear {
    std::array<CompactMonth, 12> months{};

    bool insert(unsigned month, unsigned day) { return months[month - 1].insert(day); }
    bool contains(unsigned month, unsigned day) const { return months[month - 1].contains(day); }

    std::optional<std::pair<unsigned, unsigned>> first() const {
        for (unsigned i = 0; i < 12; ++i)
            if (auto d = months[i].first()) return std::pair{i + 1, *d};
        return std::nullopt;
    }
    std::optional<std::pair<unsigned, unsigned>> first_after(unsigned month, unsigned day) const {
        if (auto d = months[month - 1].first_after(day)) return std::pair{month, *d};
        for (unsigned i = month; i < 12; ++i)
            if (auto d = months[i].first()) return std::pair{i + 1, *d};
        return std::nullopt;
    }
    unsigned count() const {
        unsigned c = 0;
        for (const auto& m : months) c += m.count();
        return c;
    }
};

struct CompactCalendar {
    int first_year = 0;
    std::deque<CompactYear> calendar;

    CompactYear* year_for_mut(NaiveDate date) {
        int64_t idx = int64_t(date.year()) - first_year;
        if (idx < 0 || idx >= int64_t(calendar.size())) return nullptr;
        return &calendar[size_t(idx)];
    }
    const CompactYear* year_for(NaiveDate date) const {
        int64_t idx = int64_t(date.year()) - first_year;
        if (idx < 0 || idx >= int64_t(calendar.size())) return nullptr;
        return &calendar[size_t(idx)];
    }

    /// Insert a day. Returns false if it was already present.
    bool insert(NaiveDate date) {
        CompactYear* y = nullptr;
        if (auto* p = year_for_mut(date)) {
            y = p;
        } else if (calendar.empty()) {
            first_year = date.year();
            calendar.emplace_back();
            y = &calendar.back();
        } else if (date.year() < first_year) {
            for (int i = date.year(); i < first_year; ++i) calendar.emplace_front();
            first_year = date.year();
            y = &calendar.front();
        } else {
            int last_year = first_year + int(calendar.size()) - 1;
            for (int i = last_year; i < date.year(); ++i) calendar.emplace_back();
            y = &calendar.back();
        }
        return y->insert(date.month(), date.day());
    }

    bool contains(NaiveDate date) const {
        if (auto* y = year_for(date)) return y->contains(date.month(), date.day());
        return false;
    }

    std::optional<NaiveDate> first_day() const {
        for (size_t i = 0; i < calendar.size(); ++i)
            if (auto md = calendar[i].first())
                return NaiveDate::ymd_unchecked(first_year + int(i), md->first, md->second);
        return std::nullopt;
    }

    /// First included day strictly after `date`, if any.
    std::optional<NaiveDate> first_after(NaiveDate date) const {
        if (auto* y = year_for(date)) {
            if (auto md = y->first_after(date.month(), date.day()))
                return NaiveDate::ymd_unchecked(date.year(), md->first, md->second);
            int64_t idx = int64_t(date.year()) - first_year;
            for (int64_t i = idx + 1; i < int64_t(calendar.size()); ++i)
                if (auto md = calendar[size_t(i)].first())
                    return NaiveDate::ymd_unchecked(first_year + int(i), md->first, md->second);
            return std::nullopt;
        } else if (date.year() < first_year) {
            return first_day();
        } else {
            return std::nullopt;
        }
    }

    unsigned count() const {
        unsigned c = 0;
        for (const auto& y : calendar) c += y.count();
        return c;
    }

    bool operator==(const CompactCalendar& o) const {
        // Compare the set of contained days, ignoring padding-year differences.
        if (first_year == o.first_year && calendar.size() == o.calendar.size()) {
            for (size_t i = 0; i < calendar.size(); ++i)
                for (unsigned m = 0; m < 12; ++m)
                    if (calendar[i].months[m].bits != o.calendar[i].months[m].bits) return false;
            return true;
        }
        return count() == o.count() && contains_all(o);
    }

  private:
    bool contains_all(const CompactCalendar& o) const {
        for (size_t i = 0; i < o.calendar.size(); ++i) {
            int yr = o.first_year + int(i);
            for (unsigned m = 1; m <= 12; ++m)
                for (unsigned d = 1; d <= 31; ++d)
                    if (o.calendar[i].contains(m, d) && !contains(NaiveDate::ymd_unchecked(yr, m, d)))
                        return false;
        }
        return true;
    }
};

// ============================================================================
// Range utilities  (opening-hours/src/utils/range.rs)
// ============================================================================

/// A half-open interval of extended times: [start, end).
struct ETRange {
    ExtendedTime start;
    ExtendedTime end;
    auto operator<=>(const ETRange&) const = default;
};

/// Intersection of two intervals, `nullopt` if they do not overlap.
inline std::optional<ETRange> range_intersection(ETRange a, ETRange b) {
    ExtendedTime s = std::max(a.start, b.start);
    ExtendedTime e = std::min(a.end, b.end);
    if (s < e) return ETRange{s, e};
    return std::nullopt;
}

/// Merge overlapping/adjacent-by-overlap intervals into a sorted disjoint set.
inline std::vector<ETRange> ranges_union(std::vector<ETRange> ranges) {
    std::sort(ranges.begin(), ranges.end(),
              [](const ETRange& a, const ETRange& b) { return a.start < b.start; });
    std::vector<ETRange> out;
    size_t i = 0;
    while (i < ranges.size()) {
        ETRange current = ranges[i++];
        while (i < ranges.size() && current.end >= ranges[i].start) {
            if (ranges[i].end > current.end) current.end = ranges[i].end;
            ++i;
        }
        out.push_back(current);
    }
    return out;
}

// ============================================================================
// Schedule  (opening-hours/src/schedule.rs)  — Milestone 1
//
// A per-day schedule: a sorted, disjoint, sparse list of `TimeRange`s (a gap
// implies closed-with-no-comment). `IntoIter` renders it into contiguous,
// hole-free ranges from 00:00 to 24:00 — the weekly-table render contract.
// ============================================================================

struct TimeRange {
    ExtendedTime start;
    ExtendedTime end;
    RuleKind kind = RuleKind::Closed;
    std::string comment;

    /// Two ranges share a state iff their (kind, comment) match.
    bool state_eq(const TimeRange& o) const { return kind == o.kind && comment == o.comment; }

    bool operator==(const TimeRange&) const = default;
};

struct Schedule {
    /// Always a sequence of non-overlapping, increasing time ranges.
    std::vector<TimeRange> inner;

    Schedule() = default;
    explicit Schedule(std::vector<TimeRange> v) : inner(std::move(v)) {}

    /// Create a schedule from ranges of a single (kind, comment); drops empties,
    /// sorts by start, and merges overlapping/adjacent ranges.
    static Schedule from_ranges(const std::vector<ETRange>& ranges, RuleKind kind,
                                const std::string& comment) {
        std::vector<TimeRange> in;
        in.reserve(ranges.size());
        for (const auto& r : ranges) {
            if (r.start < r.end) in.push_back(TimeRange{r.start, r.end, kind, comment});
        }

        if (in.size() > 1) {
            std::sort(in.begin(), in.end(),
                      [](const TimeRange& a, const TimeRange& b) { return a.start < b.start; });
            size_t i_kept = 0;
            for (size_t i_next = 1; i_next < in.size(); ++i_next) {
                if (in[i_kept].end >= in[i_next].start) {
                    in[i_kept].end = in[i_next].end;
                } else {
                    ++i_kept;
                    std::swap(in[i_kept], in[i_next]);
                }
            }
            in.resize(i_kept + 1);
        }

        return Schedule{std::move(in)};
    }

    bool is_empty() const { return inner.empty(); }

    /// True if every range is closed with an empty comment (also true if empty).
    bool is_always_closed_with_no_comments() const {
        for (const auto& rg : inner)
            if (!(rg.kind == RuleKind::Closed && rg.comment.empty())) return false;
        return true;
    }

    /// Drop closed-with-no-comment ranges (keeps closed-with-comment).
    Schedule filter_closed_ranges() const {
        Schedule out;
        for (const auto& rg : inner)
            if (rg.kind != RuleKind::Closed || !rg.comment.empty()) out.inner.push_back(rg);
        return out;
    }

    /// Insert a range: it overwrites any overlap (later insert wins), then
    /// adjacent equal-state neighbours are coalesced.
    Schedule insert(TimeRange ins) const {
        ExtendedTime ins_start = ins.start;
        ExtendedTime ins_end = ins.end;

        std::vector<TimeRange> before;
        for (const auto& tr : inner) {
            if (!(tr.start < ins_end)) continue;
            TimeRange c = tr;
            c.end = std::min(c.end, ins.start);
            if (c.start < c.end) before.push_back(std::move(c));
        }

        std::vector<TimeRange> after;
        for (const auto& tr : inner) {
            if (!(tr.end > ins_start)) continue;
            TimeRange c = tr;
            c.start = std::max(c.start, ins.end);
            if (c.start < c.end) after.push_back(std::move(c));
        }

        // Extend the inserted interval across adjacent same-state neighbours.
        while (!before.empty() && before.back().end == ins.start && before.back().state_eq(ins)) {
            ins.start = before.back().start;
            before.pop_back();
        }
        size_t ai = 0;
        while (ai < after.size() && ins.end == after[ai].start && after[ai].state_eq(ins)) {
            ins.end = after[ai].end;
            ++ai;
        }

        std::vector<TimeRange> result;
        result.reserve(before.size() + 1 + (after.size() - ai));
        for (auto& b : before) result.push_back(std::move(b));
        result.push_back(std::move(ins));
        for (size_t i = ai; i < after.size(); ++i) result.push_back(std::move(after[i]));
        return Schedule{std::move(result)};
    }

    /// Merge another schedule into this one (fold `insert` over its ranges).
    Schedule addition(const Schedule& other) const {
        Schedule acc = *this;
        for (size_t i = other.inner.size(); i-- > 0;) acc = acc.insert(other.inner[i]);
        return acc;
    }

    /// Render into contiguous, hole-free ranges covering [00:00, 24:00).
    std::vector<TimeRange> into_ranges() const {
        std::vector<TimeRange> out;
        ExtendedTime last_end = ExtendedTime::midnight_00();
        size_t i = 0;
        const auto& in = inner;
        auto is_holes = [](const TimeRange& tr) {
            return tr.kind == RuleKind::Closed && tr.comment.empty();
        };

        while (last_end < ExtendedTime::midnight_24()) {
            TimeRange yielded;
            if (i < in.size() && in[i].start == last_end) {
                yielded = in[i];
                ++i;
            } else {
                ExtendedTime start = last_end;
                ExtendedTime end = (i < in.size()) ? in[i].start : start;
                yielded = TimeRange{start, end, RuleKind::Closed, std::string()};
            }

            bool stopped = false;
            while (i < in.size()) {
                const TimeRange& next_range = in[i];
                if (next_range.start > yielded.end) {
                    if (is_holes(yielded)) {
                        yielded.end = next_range.start;
                    } else {
                        stopped = true;
                        break;
                    }
                }
                if (yielded.state_eq(next_range)) {
                    yielded.end = next_range.end;
                    ++i;
                } else {
                    stopped = true;
                    break;
                }
            }

            if (!stopped && is_holes(yielded)) yielded.end = ExtendedTime::midnight_24();

            last_end = yielded.end;
            out.push_back(std::move(yielded));
        }

        return out;
    }

    bool operator==(const Schedule&) const = default;
};

/// A lazily-peekable cursor over `Schedule::into_ranges()`.
///
/// The day's contiguous ranges are materialised up-front (a day has only a
/// handful of intervals), then served with `peek()` / `next()` — matching the
/// `Peekable<IntoIter>` the Rust `TimeDomainIterator` drives.
class ScheduleIter {
    std::vector<TimeRange> items_;
    size_t idx_ = 0;

  public:
    ScheduleIter() = default;
    explicit ScheduleIter(const Schedule& s) : items_(s.into_ranges()) {}

    const TimeRange* peek() const { return idx_ < items_.size() ? &items_[idx_] : nullptr; }
    bool has_next() const { return idx_ < items_.size(); }

    std::optional<TimeRange> next() {
        if (idx_ >= items_.size()) return std::nullopt;
        return items_[idx_++];
    }
};

// ============================================================================
// DateTimeRange  (opening-hours/src/utils/range.rs)
// ============================================================================

/// An interval of constant state over absolute time.
template <class D = NaiveDateTime>
struct DateTimeRange {
    D start;
    D end;
    RuleKind kind = RuleKind::Closed;
    std::string comment;

    std::pair<RuleKind, std::string> into_state() const { return {kind, comment}; }
};

// ============================================================================
// Localization  (opening-hours/src/localization/*)  — Milestones 6 & 8
//
//   Coordinates  — validated lat/lon + NOAA solar-event math (sunrise v3.0.0).
//   Localize     — the interface every locale implements (a C++ concept).
//   NoLocation   — the default: identity time math + fixed sun-event fallback.
//   TzLocation<Tz> — timezone-aware, generic over a consumer-supplied
//                    tz-provider (we deliberately do NOT bind a concrete tz DB).
// ============================================================================

// --- Coordinates + solar math (port of the `sunrise` crate v3.0.0) ---

namespace detail {

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double J2000 = 2451545.0;
inline constexpr double UNIX_EPOCH_JULIAN_DAY = 2440587.5;
inline constexpr double SECONDS_IN_A_DAY = 86400.0;

inline double deg2rad(double d) { return d * kPi / 180.0; }

inline double rem_euclid(double lhs, double rhs) {
    double res = std::fmod(lhs, rhs);
    return (res < 0.0) ? res + std::abs(rhs) : res;
}

/// Julian day of local mean solar noon at a given longitude (degrees) & date.
inline double mean_solar_noon(double lon, NaiveDate date) {
    // Unix timestamp of that date at 12:00 UTC.
    double noon_ts = double(date.day_number()) * SECONDS_IN_A_DAY + 12.0 * 3600.0;
    double julian = noon_ts / SECONDS_IN_A_DAY + UNIX_EPOCH_JULIAN_DAY;
    return julian - lon / 360.0;
}

inline double solar_mean_anomaly(double day) {
    return rem_euclid(deg2rad(357.5291 + 0.98560028 * (day - J2000)), 2.0 * kPi);
}

inline double equation_of_center(double m) {
    return deg2rad(1.9148 * std::sin(m) + 0.02 * std::sin(2.0 * m) + 0.0003 * std::sin(3.0 * m));
}

inline double argument_of_perihelion(double day) {
    return deg2rad(102.93005 + 0.3179526 * (day - J2000) / 36525.0);
}

inline double ecliptic_longitude(double m, double eoc, double day) {
    return std::fmod(m + eoc + std::fmod(argument_of_perihelion(day), 2.0 * kPi) + 3.0 * kPi,
                     2.0 * kPi);
}

inline double solar_transit(double day, double m, double lambda) {
    return day + (0.0053 * std::sin(m) - 0.0069 * std::sin(2.0 * lambda));
}

inline double declination(double lambda) { return std::asin(std::sin(lambda) * 0.39779); }

/// The elevation angle (radians) that defines a given solar event.
inline double event_angle(TimeEvent event) {
    switch (event) {
    case TimeEvent::Sunrise:
    case TimeEvent::Sunset:
        return deg2rad(5.0) / 6.0; // ~0.8333°
    case TimeEvent::Dawn:
    case TimeEvent::Dusk:
        return deg2rad(6.0); // civil
    }
    return 0.0;
}

inline bool event_is_morning(TimeEvent event) {
    return event == TimeEvent::Sunrise || event == TimeEvent::Dawn;
}

/// Hour angle for a solar event (altitude fixed at 0). `nullopt` => no such
/// event happens on that day (polar day / night).
inline std::optional<double> hour_angle(double latitude_deg, double decl, TimeEvent event) {
    double latitude = deg2rad(latitude_deg);
    double denom = std::cos(latitude) * std::cos(decl);
    // altitude == 0, so the altitude-correction term vanishes.
    double numer = -std::sin(event_angle(event)) - std::sin(latitude) * std::sin(decl);
    double ratio = numer / denom;
    // The sun never reaches the event elevation on a polar day/night, i.e. the
    // ratio falls outside [-1, 1]. Test the domain with finite arithmetic rather
    // than relying on std::acos() returning NaN: -ffast-math / -ffinite-math-only
    // (release builds) assume NaN cannot occur and optimize an isnan() check
    // away, letting the NaN leak through as a bogus event time.
    if (!(ratio >= -1.0 && ratio <= 1.0))
        return std::nullopt;
    double sign = event_is_morning(event) ? -1.0 : 1.0;
    return sign * std::acos(ratio);
}

} // namespace detail

/// A validated pair of geographic coordinates (degrees).
struct Coordinates {
    double lat_ = 0.0;
    double lon_ = 0.0;

    /// Validate lat/lon; `nullopt` for NaN or out-of-range values.
    static std::optional<Coordinates> make(double lat, double lon) {
        if (std::isnan(lat) || std::isnan(lon) || lat < -90.0 || lat > 90.0 || lon < -180.0 ||
            lon > 180.0)
            return std::nullopt;
        return Coordinates{lat, lon};
    }

    double lat() const { return lat_; }
    double lon() const { return lon_; }

    /// UTC instant (unix seconds) of a solar event on `date`, `nullopt` if the
    /// event does not occur (polar day/night).
    std::optional<int64_t> event_time_utc(NaiveDate date, TimeEvent event) const {
        using namespace detail;
        double day = mean_solar_noon(lon_, date);
        double m = solar_mean_anomaly(day);
        double eoc = equation_of_center(m);
        double lambda = ecliptic_longitude(m, eoc, day);
        double transit = solar_transit(day, m, lambda);
        double decl = declination(lambda);

        std::optional<double> ha = hour_angle(lat_, decl, event);
        if (!ha) return std::nullopt;

        double frac = *ha / (2.0 * kPi);
        double jd = transit + frac;
        // julian_to_unix, truncated toward zero (matches Rust `as i64`).
        return int64_t((jd - UNIX_EPOCH_JULIAN_DAY) * SECONDS_IN_A_DAY);
    }

    auto operator<=>(const Coordinates&) const = default;
    bool operator==(const Coordinates&) const = default;
};

/// Fixed local fallback sun times when no coordinates are available
/// (localize.rs:23-32): dawn 06:00, sunrise 07:00, sunset 19:00, dusk 20:00.
inline ExtendedTime fixed_event_fallback(TimeEvent event) {
    switch (event) {
    case TimeEvent::Dawn:
        return ExtendedTime{6, 0};
    case TimeEvent::Sunrise:
        return ExtendedTime{7, 0};
    case TimeEvent::Sunset:
        return ExtendedTime{19, 0};
    case TimeEvent::Dusk:
        return ExtendedTime{20, 0};
    }
    return ExtendedTime{0, 0};
}

/// The interface every locale must implement. Evaluation runs in naive local
/// time; boundaries are mapped back through `datetime` so results stay
/// DST-correct.
template <class L>
concept Localize =
    requires(const L& l, typename L::DateTime dt, NaiveDateTime naive, NaiveDate date, TimeEvent ev) {
        typename L::DateTime;
        { l.naive(dt) } -> std::same_as<NaiveDateTime>;
        { l.datetime(naive) } -> std::same_as<typename L::DateTime>;
        { l.event_time(date, ev) } -> std::same_as<std::optional<ExtendedTime>>;
    };

/// No location info: identity time math, fixed sun-event fallback.
struct NoLocation {
    using DateTime = NaiveDateTime;

    NaiveDateTime naive(NaiveDateTime dt) const { return dt; }
    NaiveDateTime datetime(NaiveDateTime naive) const { return naive; }
    std::optional<ExtendedTime> event_time(NaiveDate /*date*/, TimeEvent event) const {
        return fixed_event_fallback(event);
    }

    auto operator<=>(const NoLocation&) const = default;
    bool operator==(const NoLocation&) const = default;
};

/// Result of mapping a naive local time back to absolute instant(s).
/// Mirrors `chrono::LocalResult` / `TimeZone::from_local_datetime`.
struct LocalResolution {
    enum class Kind { None, Single, Ambiguous };
    Kind kind = Kind::None;
    int64_t earliest = 0; // unix seconds (valid if Single/Ambiguous)
    int64_t latest = 0;   // unix seconds (valid if Single/Ambiguous)

    static LocalResolution none() { return {Kind::None, 0, 0}; }
    static LocalResolution single(int64_t utc) { return {Kind::Single, utc, utc}; }
    static LocalResolution ambiguous(int64_t earliest, int64_t latest) {
        return {Kind::Ambiguous, earliest, latest};
    }
};

/// The datetime type carried by `TzLocation`: an absolute instant (unix
/// seconds). The owning `TzLocation` knows how to project it to/from local.
struct ZonedDateTime {
    int64_t utc_seconds = 0;

    ZonedDateTime add_minutes(int64_t m) const { return {utc_seconds + m * 60}; }
    auto operator<=>(const ZonedDateTime&) const = default;
    bool operator==(const ZonedDateTime&) const = default;
};

/// A consumer-supplied timezone provider. Two mappings are required:
///   NaiveDateTime  to_local(int64_t utc_seconds) const;    // instant -> local
///   LocalResolution from_local(NaiveDateTime local) const; // local -> instant(s)
template <class Tz>
concept TzProvider = requires(const Tz& tz, int64_t utc, NaiveDateTime local) {
    { tz.to_local(utc) } -> std::same_as<NaiveDateTime>;
    { tz.from_local(local) } -> std::same_as<LocalResolution>;
};

/// A timezone-aware locale, generic over the tz-provider (localize.rs:136-167).
/// Optionally carries coordinates for accurate sun events.
template <class Tz>
    requires TzProvider<Tz>
struct TzLocation {
    using DateTime = ZonedDateTime;

    Tz tz;
    std::optional<Coordinates> coords;

    explicit TzLocation(Tz tz_) : tz(std::move(tz_)) {}
    TzLocation(Tz tz_, Coordinates c) : tz(std::move(tz_)), coords(c) {}

    TzLocation with_coords(Coordinates c) const { return TzLocation{tz, c}; }
    const std::optional<Coordinates>& get_coords() const { return coords; }
    const Tz& get_timezone() const { return tz; }

    NaiveDateTime naive(ZonedDateTime dt) const { return tz.to_local(dt.utc_seconds); }

    /// Local -> absolute, choosing the latest offset on ambiguous times and
    /// stepping forward one minute at a time across spring-forward gaps.
    ZonedDateTime datetime(NaiveDateTime naive) const {
        while (true) {
            LocalResolution r = tz.from_local(naive);
            if (r.kind != LocalResolution::Kind::None) return ZonedDateTime{r.latest};
            naive = naive.add_minutes(1);
        }
    }

    std::optional<ExtendedTime> event_time(NaiveDate date, TimeEvent event) const {
        if (!coords) return fixed_event_fallback(event);
        auto utc = coords->event_time_utc(date, event);
        if (!utc) return std::nullopt;
        NaiveDateTime local = tz.to_local(*utc);
        return ExtendedTime{uint8_t(local.hour()), uint8_t(local.minute_of_hour())};
    }

    bool operator==(const TzLocation&) const = default;
};

// ============================================================================
// Context  (opening-hours/src/context.rs)
// ============================================================================

/// A pair of holiday calendars (public + school). Empty by default, in which
/// case holiday lookups gracefully return `false` ("PH off" -> never off).
struct ContextHolidays {
    std::shared_ptr<const CompactCalendar> public_;
    std::shared_ptr<const CompactCalendar> school;

    ContextHolidays() = default;
    ContextHolidays(std::shared_ptr<const CompactCalendar> pub,
                    std::shared_ptr<const CompactCalendar> sch)
        : public_(std::move(pub)), school(std::move(sch)) {}

    const CompactCalendar& get_for_kind(HolidayKind kind) const {
        static const CompactCalendar kEmpty;
        const auto& p = (kind == HolidayKind::Public) ? public_ : school;
        return p ? *p : kEmpty;
    }
};

/// All context attached to an expression that affects its evaluation.
template <class L = NoLocation>
    requires Localize<L>
struct Context {
    ContextHolidays holidays;
    ContextHolidays holidays_unknown;
    L locale{};

    Context() = default;
    Context(ContextHolidays h, ContextHolidays hu, L loc)
        : holidays(std::move(h)), holidays_unknown(std::move(hu)), locale(std::move(loc)) {}

    Context with_holidays(ContextHolidays h) const {
        Context c = *this;
        c.holidays = std::move(h);
        return c;
    }
    Context with_holidays_unknown(ContextHolidays hu) const {
        Context c = *this;
        c.holidays_unknown = std::move(hu);
        return c;
    }
    template <class L2>
        requires Localize<L2>
    Context<L2> with_locale(L2 loc) const {
        return Context<L2>{holidays, holidays_unknown, std::move(loc)};
    }
};

// ============================================================================
// ExtendedTime arithmetic helpers (extended_time.rs)
// ============================================================================

/// Add plain minutes; `nullopt` if the result leaves [00:00, 48:00].
inline std::optional<ExtendedTime> et_add_minutes(ExtendedTime t, int minutes) {
    int total = int(t.mins_from_midnight()) + minutes;
    if (total < 0 || total > 48 * 60) return std::nullopt;
    return ExtendedTime::from_mins(uint16_t(total));
}

/// Add whole hours, assumed in range (used for the ≤24h past-midnight wrap).
inline ExtendedTime et_add_hours(ExtendedTime t, int hours) {
    return ExtendedTime::from_mins(uint16_t(int(t.mins_from_midnight()) + hours * 60)).value();
}

// ============================================================================
// Time filter  (opening-hours/src/filter/time_filter.rs)  — Milestone 3
//
// Projects a `TimeSelector` onto a concrete day: resolves fixed/sun times,
// wraps past-midnight spans, and applies the seasonal sun fallback.
// ============================================================================

/// True iff every span is exactly 00:00-24:00 (`is_immutable_full_day`).
inline bool time_selector_is_immutable_full_day(const TimeSelector& ts) {
    TimeSpan full = TimeSpan::fixed_range(ExtendedTime::midnight_00(), ExtendedTime::midnight_24());
    for (const auto& span : ts.time)
        if (!(span == full)) return false;
    return true;
}

/// Resolve a `Time` to a concrete extended time (`nullopt` for a sun event that
/// does not occur that day).
template <class L>
    requires Localize<L>
std::optional<ExtendedTime> time_as_naive(const Time& t, const Context<L>& ctx, NaiveDate date) {
    if (t.tag == Time::Fixed) return t.fixed;
    auto ev = ctx.locale.event_time(date, t.variable.event);
    if (!ev) return std::nullopt;
    return et_add_minutes(*ev, t.variable.offset);
}

/// Project a `TimeSpan` onto a day as a naive interval (time_filter.rs:73-146).
/// Handles the past-midnight wrap and the polar-day/night seasonal fallback.
template <class L>
    requires Localize<L>
std::optional<ETRange> timespan_as_naive(const TimeSpan& span, const Context<L>& ctx,
                                         NaiveDate date) {
    auto start_opt = time_as_naive(span.start, ctx, date);
    auto end_opt = time_as_naive(span.end, ctx, date);
    std::pair<unsigned, unsigned> md{date.month(), date.day()};
    bool is_summer = md >= std::pair<unsigned, unsigned>{3, 20} &&
                     md < std::pair<unsigned, unsigned>{9, 22};

    ExtendedTime start{}, end{};
    if (start_opt && end_opt) {
        start = *start_opt;
        end = *end_opt;
    } else if (!start_opt && end_opt) {
        // Missing start: it must be a (non-occurring) sun event.
        if (span.start.tag != Time::Variable) return std::nullopt;
        bool is_morning = span.start.variable.event == TimeEvent::Sunrise ||
                          span.start.variable.event == TimeEvent::Dawn;
        if (is_morning == is_summer)
            start = ExtendedTime::midnight_00();
        else
            return std::nullopt;
        end = *end_opt;
    } else if (start_opt && !end_opt) {
        if (span.end.tag != Time::Variable) return std::nullopt;
        bool is_morning = span.end.variable.event == TimeEvent::Sunrise ||
                          span.end.variable.event == TimeEvent::Dawn;
        if (is_morning == is_summer)
            return std::nullopt;
        else
            end = ExtendedTime::midnight_24();
        start = *start_opt;
    } else {
        // Both missing: full day iff the ordering agrees with the season.
        if ((span.start <= span.end) == is_summer) {
            start = ExtendedTime::midnight_00();
            end = ExtendedTime::midnight_24();
        } else {
            return std::nullopt;
        }
    }

    // If end <= start the span wraps into the next day.
    if (!(start < end)) end = et_add_hours(end, 24);
    return ETRange{start, end};
}

/// Intervals of a time selector clipped to the current day [00:00, 24:00).
template <class L>
    requires Localize<L>
std::vector<ETRange> time_selector_intervals_at(const Context<L>& ctx, const TimeSelector& ts,
                                                NaiveDate date) {
    std::vector<ETRange> ranges;
    ETRange full_day{ExtendedTime::midnight_00(), ExtendedTime::midnight_24()};
    for (const auto& span : ts.time) {
        auto r = timespan_as_naive(span, ctx, date);
        if (!r) continue;
        if (auto clipped = range_intersection(*r, full_day)) ranges.push_back(*clipped);
    }
    return ranges_union(std::move(ranges));
}

/// Intervals of a time selector that spill into [24:00, 48:00), shifted −24h so
/// they can be merged into the *following* day.
template <class L>
    requires Localize<L>
std::vector<ETRange> time_selector_intervals_at_next_day(const Context<L>& ctx,
                                                         const TimeSelector& ts, NaiveDate date) {
    std::vector<ETRange> ranges;
    ETRange next_day{ExtendedTime::midnight_24(), ExtendedTime::midnight_48()};
    for (const auto& span : ts.time) {
        auto r = timespan_as_naive(span, ctx, date);
        if (!r) continue;
        auto clipped = range_intersection(*r, next_day);
        if (!clipped) continue;
        ExtendedTime start = et_add_hours(clipped->start, -24);
        ExtendedTime end = et_add_hours(clipped->end, -24);
        ranges.push_back(ETRange{start, end});
    }
    return ranges_union(std::move(ranges));
}

// ============================================================================
// Date filter  (opening-hours/src/filter/date_filter.rs)  — Milestone 4
//
// `day_selector_filter(date)` = AND across year/monthday/week/weekday, where an
// empty selector list matches all dates. Each selector also provides a
// `next_change_hint` lower bound (a performance skip) and holidays can force a
// rule's kind to Unknown.
// ============================================================================

/// An inclusive date interval [first, last].
struct DateInterval {
    NaiveDate first;
    NaiveDate last;
    bool contains(NaiveDate d) const { return first <= d && d <= last; }
};

/// `Option<NaiveDate>` min with Rust's `None < Some` ordering (None == "check
/// day-by-day", so it dominates).
inline std::optional<NaiveDate> min_opt(std::optional<NaiveDate> a, std::optional<NaiveDate> b) {
    if (!a || !b) return std::nullopt;
    return std::min(*a, *b);
}

// --- valid-date clamping (date_filter.rs:14-44) ---

/// First valid date at or before y/m/d (e.g. Feb 30 → Feb 28).
inline NaiveDate valid_ymd_before(int year, unsigned month, unsigned day) {
    if (auto d = NaiveDate::from_ymd(year, month, day)) return *d;
    for (unsigned dd = day - 1; dd >= 28; --dd)
        if (auto d = NaiveDate::from_ymd(year, month, dd)) return *d;
    return DATE_END_DATE;
}

/// First valid date at or after y/m/d (e.g. Feb 30 → Mar 1).
inline NaiveDate valid_ymd_after(int year, unsigned month, unsigned day) {
    if (auto d = NaiveDate::from_ymd(year, month, day)) return *d;
    for (unsigned dd = day - 1; dd >= 28; --dd)
        if (auto d = NaiveDate::from_ymd(year, month, dd)) return d->succ();
    return DATE_END_DATE;
}

using DateBuilder = NaiveDate (*)(int, unsigned, unsigned);

// --- interval bound machinery (date_filter.rs:46-125) ---

inline std::vector<NaiveDate> ensure_increasing(std::vector<NaiveDate> v) {
    std::vector<NaiveDate> out;
    size_t i = 0;
    while (i < v.size()) {
        NaiveDate val = v[i++];
        while (i < v.size() && v[i] <= val) ++i;
        out.push_back(val);
    }
    return out;
}

inline std::vector<DateInterval> intervals_from_bounds(std::vector<NaiveDate> starts_in,
                                                       std::vector<NaiveDate> ends_in) {
    auto starts = ensure_increasing(std::move(starts_in));
    auto ends = ensure_increasing(std::move(ends_in));
    bool start_is_empty = starts.empty();
    size_t si = 0, ei = 0;
    std::vector<DateInterval> out;

    while (true) {
        if (si < starts.size()) {
            NaiveDate start = starts[si];
            while (ei < ends.size() && ends[ei] < start) ++ei;
        }
        bool has_start = si < starts.size();
        bool has_end = ei < ends.size();

        if (!has_start && !has_end) break;

        DateInterval range;
        if (!has_start && has_end) {
            if (start_is_empty) {
                range = {DATE_START_DATE, ends[ei]};
                ei = ends.size();
            } else {
                break;
            }
        } else if (has_start && !has_end) {
            range = {starts[si], DATE_END_DATE};
            ++si;
        } else {
            NaiveDate start = starts[si];
            NaiveDate end = ends[ei];
            if (start <= end) {
                if (start == end) ++ei;
                ++si;
                range = {start, end};
            } else {
                break; // unreachable in the reference
            }
        }
        out.push_back(range);
    }
    return out;
}

inline bool is_open_from_intervals(NaiveDate date, const std::vector<DateInterval>& intervals) {
    for (const auto& rg : intervals)
        if (rg.last >= date) return rg.contains(date);
    return false;
}

inline NaiveDate next_change_from_intervals(NaiveDate date,
                                            const std::vector<DateInterval>& intervals) {
    for (const auto& rg : intervals) {
        if (rg.last >= date) {
            if (rg.first <= date) return rg.last.succ();
            return rg.first;
        }
    }
    return DATE_END_DATE;
}

// --- date projection (date_filter.rs:263-320) ---

inline std::optional<NaiveDate> date_for_nth_wday(int year, Month month, Weekday wday, int nth) {
    if (nth >= 0) return from_weekday_of_month(year, unsigned(month), wday, nth);
    int num = from_weekday_of_month(year, unsigned(month), wday, 5).has_value() ? 5 : 4;
    int idx = (num + 1) - std::abs(nth);
    if (idx < 0) idx = 0;
    return from_weekday_of_month(year, unsigned(month), wday, idx);
}

inline std::optional<NaiveDate> date_on_year(const Date& date, int for_year, DateBuilder builder) {
    switch (date.kind) {
    case DateKind::Easter:
        return easter(date.year.has_value() ? int(*date.year) : for_year);
    case DateKind::Fixed:
        if (date.year.has_value() && int(*date.year) != for_year) return std::nullopt;
        return builder(for_year, unsigned(date.month), date.day);
    case DateKind::WeekdayInMonth:
        if (date.year.has_value() && int(*date.year) != for_year) return std::nullopt;
        return date_for_nth_wday(for_year, date.month, date.weekday, date.nth);
    }
    return std::nullopt;
}

// --- MonthdayRange -> intervals (date_filter.rs:322-401) ---

inline std::vector<DateInterval> monthday_range_to_intervals(NaiveDate date,
                                                             const MonthdayRange& mr) {
    std::vector<DateInterval> out;

    if (mr.kind == MonthdayRangeKind::MonthOnly) {
        if (!mr.month_year.has_value()) {
            // Month range, no explicit year.
            std::vector<NaiveDate> starts, ends;
            for (int y = date.year() - 1; y <= date.year() + 1; ++y) {
                if (auto d = NaiveDate::from_ymd(y, unsigned(mr.month_start), 1))
                    starts.push_back(*d);
            }
            for (int y = date.year() - 1; y <= date.year() + 1; ++y) {
                if (auto d = NaiveDate::from_ymd(y, unsigned(next_month(mr.month_end)), 1))
                    ends.push_back(d->pred());
            }
            return intervals_from_bounds(std::move(starts), std::move(ends));
        } else {
            // Month range with an explicit start year.
            int year = int(*mr.month_year);
            for (int end_year : {year, year + 1}) {
                auto start = NaiveDate::from_ymd(year, unsigned(mr.month_start), 1);
                auto end = NaiveDate::from_ymd(end_year, unsigned(mr.month_end), 1);
                if (start && end && start->d <= end->d) {
                    out.push_back({*start, *end});
                    break;
                }
            }
            return out;
        }
    }

    // DateRange.
    bool has_year = mr.date_start.year.has_value() || mr.date_end.year.has_value();
    if (has_year) {
        int year = mr.date_start.year.has_value() ? int(*mr.date_start.year) : int(*mr.date_end.year);
        std::pair<int, int> combos[3] = {{year, year}, {year - 1, year}, {year, year + 1}};
        for (auto [ys, ye] : combos) {
            int year_start = mr.date_start.year.has_value() ? int(*mr.date_start.year) : ys;
            int year_end = mr.date_end.year.has_value() ? int(*mr.date_end.year) : ye;
            auto sd = date_on_year(mr.date_start, year_start, valid_ymd_after);
            if (!sd) continue;
            NaiveDate s = apply_date_offset(mr.offset_start, *sd);
            auto ed = date_on_year(mr.date_end, year_end, valid_ymd_before);
            if (!ed) continue;
            NaiveDate e = apply_date_offset(mr.offset_end, *ed);
            if (s <= e) {
                out.push_back({s, e});
                break;
            }
        }
        return out;
    }

    // DateRange, no explicit year.
    int year = date.year();
    Date feb29 = Date::make_fixed(std::nullopt, Month::Feb, 29);
    if (mr.date_start == feb29 && mr.date_end == feb29) {
        for (int y = year - 4; y <= year + 4; ++y) {
            if (auto d = NaiveDate::from_ymd(y, 2, 29)) {
                NaiveDate s = apply_date_offset(mr.offset_start, *d);
                NaiveDate e = apply_date_offset(mr.offset_end, *d);
                out.push_back({s, e});
            }
        }
        return out;
    }

    std::vector<NaiveDate> starts, ends;
    for (int y = year - 1; y <= year + 1; ++y)
        if (auto d = date_on_year(mr.date_start, y, valid_ymd_after))
            starts.push_back(apply_date_offset(mr.offset_start, *d));
    for (int y = year - 1; y <= year + 1; ++y)
        if (auto d = date_on_year(mr.date_end, y, valid_ymd_before))
            ends.push_back(apply_date_offset(mr.offset_end, *d));
    return intervals_from_bounds(std::move(starts), std::move(ends));
}

// --- YearRange (date_filter.rs:217-261) ---

inline bool year_filter(const YearRange& yr, NaiveDate date) {
    int year = date.year();
    int start = yr.start, end = yr.end, step = yr.step;
    return start <= year && year <= end && (year - start) % step == 0;
}

inline std::optional<NaiveDate> year_next_change_hint(const YearRange& yr, NaiveDate date) {
    int curr = date.year();
    int start = yr.start, end = yr.end, step = yr.step;

    int next_year;
    if (end < curr) {
        return DATE_END_DATE;
    } else if (curr < start) {
        next_year = start;
    } else if (step == 1) {
        next_year = end + 1;
    } else if ((curr - start) % step == 0) {
        next_year = curr + 1;
    } else {
        auto round_up = [](int x, int d) { return d * ((x + d - 1) / d); };
        next_year = start + round_up(curr - start, step);
    }

    if (auto d = NaiveDate::from_ymd(next_year, 1, 1)) return *d;
    return DATE_END_DATE;
}

// --- MonthdayRange (date_filter.rs:403-420) ---

inline bool monthday_filter(const MonthdayRange& mr, NaiveDate date) {
    return is_open_from_intervals(date, monthday_range_to_intervals(date, mr));
}

inline std::optional<NaiveDate> monthday_next_change_hint(const MonthdayRange& mr, NaiveDate date) {
    return next_change_from_intervals(date, monthday_range_to_intervals(date, mr));
}

// --- WeekRange (date_filter.rs:509-557) ---

inline bool week_filter(const WeekRange& wr, NaiveDate date) {
    int week = iso_week(date).week;
    int start = wr.start, end = wr.end, step = wr.step;
    if (!(start <= week && week <= end)) return false;
    int sat = (week < start) ? 0 : (week - start); // saturating_sub
    return sat % step == 0;
}

inline std::optional<NaiveDate> week_next_change_hint(const WeekRange& wr, NaiveDate date) {
    IsoWeek iw = iso_week(date);
    int week = iw.week;
    int start = wr.start, end = wr.end, step = wr.step;
    if (start > end) return std::nullopt;

    int weeknum;
    if (start <= week && week <= end) {
        if (step == 1) {
            weeknum = end % 54 + 1;
        } else if ((week - start) % step == 0) {
            weeknum = (week % 54) + 1;
        } else {
            return std::nullopt;
        }
    } else {
        weeknum = start;
    }

    auto res_opt = from_isoywd(iw.year, weeknum, Weekday::Mon);
    if (!res_opt) return std::nullopt;
    NaiveDate res = *res_opt;
    while (res <= date) {
        auto nxt = from_isoywd(iso_week(res).year + 1, weeknum, Weekday::Mon);
        if (!nxt) return std::nullopt;
        res = *nxt;
    }
    return res;
}

// --- WeekDayRange (date_filter.rs:422-507) ---

inline bool weekday_fixed_filter_nowrap(Weekday rs, Weekday re, int64_t offset, const bool nfs[5],
                                        const bool nfe[5], NaiveDate date) {
    NaiveDate d = date.add_days(-offset);
    int pos_from_start = (int(d.day()) - 1) / 7;
    int pos_from_end = (int(count_days_in_month(d)) - int(d.day())) / 7;
    int wd = d.weekday_index();
    bool in_range = int(rs) <= wd && wd <= int(re);
    return in_range && (nfs[pos_from_start] || nfe[pos_from_end]);
}

template <class L>
    requires Localize<L>
bool weekday_filter(const Context<L>& ctx, const WeekDayRange& w, NaiveDate date) {
    if (w.kind == WeekDayRangeKind::FixedDays) {
        if (int(w.range_start) > int(w.range_end)) {
            return weekday_fixed_filter_nowrap(w.range_start, Weekday::Sun, w.offset, w.nth_from_start,
                                               w.nth_from_end, date) ||
                   weekday_fixed_filter_nowrap(Weekday::Mon, w.range_end, w.offset, w.nth_from_start,
                                               w.nth_from_end, date);
        }
        return weekday_fixed_filter_nowrap(w.range_start, w.range_end, w.offset, w.nth_from_start,
                                           w.nth_from_end, date);
    }
    NaiveDate d = date.add_days(-w.offset);
    return ctx.holidays.get_for_kind(w.holiday_kind).contains(d) ||
           ctx.holidays_unknown.get_for_kind(w.holiday_kind).contains(d);
}

template <class L>
    requires Localize<L>
bool weekday_overrides_unknown(const Context<L>& ctx, const WeekDayRange& w, NaiveDate date) {
    if (w.kind != WeekDayRangeKind::Holiday) return false;
    NaiveDate d = date.add_days(-w.offset);
    return ctx.holidays_unknown.get_for_kind(w.holiday_kind).contains(d);
}

template <class L>
    requires Localize<L>
std::optional<NaiveDate> weekday_next_change_hint(const Context<L>& ctx, const WeekDayRange& w,
                                                  NaiveDate date) {
    if (w.kind != WeekDayRangeKind::Holiday) return std::nullopt;

    NaiveDate date_with_offset = date.add_days(-w.offset);
    auto next_change_for = [&](const CompactCalendar& cal) -> NaiveDate {
        if (cal.contains(date_with_offset)) {
            return date.succ();
        }
        if (auto following = cal.first_after(date_with_offset))
            return following->add_days(w.offset);
        return DATE_END_DATE;
    };

    NaiveDate a = next_change_for(ctx.holidays.get_for_kind(w.holiday_kind));
    NaiveDate b = next_change_for(ctx.holidays_unknown.get_for_kind(w.holiday_kind));
    return std::min(a, b);
}

// --- DaySelector combinators (date_filter.rs:154-215) ---

template <class Vec, class F>
bool vec_filter(const Vec& v, F&& f) {
    if (v.empty()) return true;
    for (const auto& x : v)
        if (f(x)) return true;
    return false;
}

template <class Vec, class F>
bool vec_any(const Vec& v, F&& f) {
    for (const auto& x : v)
        if (f(x)) return true;
    return false;
}

template <class Vec, class F>
std::optional<NaiveDate> vec_hint(const Vec& v, F&& f) {
    if (v.empty()) return DATE_END_DATE;
    std::optional<NaiveDate> best;
    bool first = true;
    for (const auto& x : v) {
        std::optional<NaiveDate> h = f(x);
        if (first) {
            best = h;
            first = false;
        } else {
            best = min_opt(best, h);
        }
    }
    return best;
}

template <class L>
    requires Localize<L>
bool day_selector_filter(const Context<L>& ctx, const DaySelector& ds, NaiveDate date) {
    return vec_filter(ds.year, [&](const YearRange& x) { return year_filter(x, date); }) &&
           vec_filter(ds.monthday, [&](const MonthdayRange& x) { return monthday_filter(x, date); }) &&
           vec_filter(ds.week, [&](const WeekRange& x) { return week_filter(x, date); }) &&
           vec_filter(ds.weekday,
                      [&](const WeekDayRange& x) { return weekday_filter(ctx, x, date); });
}

template <class L>
    requires Localize<L>
bool day_selector_overrides_unknown(const Context<L>& ctx, const DaySelector& ds, NaiveDate date) {
    // Only weekday-holiday selectors can force Unknown.
    return vec_any(ds.weekday,
                   [&](const WeekDayRange& x) { return weekday_overrides_unknown(ctx, x, date); });
}

template <class L>
    requires Localize<L>
std::optional<NaiveDate> day_selector_next_change_hint(const Context<L>& ctx, const DaySelector& ds,
                                                       NaiveDate date) {
    if (ds.is_empty()) return DATE_END_DATE;
    auto h = vec_hint(ds.year, [&](const YearRange& x) { return year_next_change_hint(x, date); });
    h = min_opt(h, vec_hint(ds.monthday,
                            [&](const MonthdayRange& x) { return monthday_next_change_hint(x, date); }));
    h = min_opt(h, vec_hint(ds.week, [&](const WeekRange& x) { return week_next_change_hint(x, date); }));
    h = min_opt(h, vec_hint(ds.weekday, [&](const WeekDayRange& x) {
                    return weekday_next_change_hint(ctx, x, date);
                }));
    return h;
}

// ============================================================================
// Expression helpers
// ============================================================================

/// Single comment for a rule (the C++ AST stores ≤2 comments; join to match the
/// Rust `Arc<str>` single comment).
inline std::string rule_comment(const RuleSequence& rs) {
    std::string out;
    for (size_t i = 0; i < rs.comments.size(); ++i) {
        if (i) out += ", ";
        out += rs.comments[i];
    }
    return out;
}

/// `RuleSequence::as_state()` — the (kind, comment) pair defining current state.
inline std::pair<RuleKind, std::string> rule_state(const RuleSequence& rs) {
    return {rs.kind, rule_comment(rs)};
}

/// `OpeningHoursExpression::is_constant()` (rules/mod.rs:33-48).
inline bool expr_is_constant(const OpeningHoursExpression& expr) {
    if (expr.rules.empty()) return true;
    auto state = rule_state(expr.rules.back());

    // Find, scanning from the end, the first "interesting" rule.
    const RuleSequence* tail = nullptr;
    for (auto it = expr.rules.rbegin(); it != expr.rules.rend(); ++it) {
        if (it->day_selector.is_empty() || !it->time_selector.is_00_24() ||
            rule_state(*it) != state) {
            tail = &*it;
            break;
        }
    }

    if (tail == nullptr) {
        return state == std::pair<RuleKind, std::string>{RuleKind::Closed, std::string()};
    }
    return rule_state(*tail) == state && tail->is_constant();
}

// ============================================================================
// Evaluator core  (opening-hours/src/opening_hours.rs)  — Milestones 2 & 5
// ============================================================================

/// Build the schedule for a single rule sequence at a date, merging today's
/// intervals with yesterday's past-midnight spill (opening_hours.rs:374-433).
template <class L>
    requires Localize<L>
std::optional<Schedule> rule_sequence_schedule_at(const RuleSequence& rs, NaiveDate date,
                                                  const Context<L>& ctx) {
    auto build = [&](NaiveDate d, const std::vector<ETRange>& intervals) -> std::optional<Schedule> {
        if (!day_selector_filter(ctx, rs.day_selector, d)) return std::nullopt;
        RuleKind kind =
            day_selector_overrides_unknown(ctx, rs.day_selector, d) ? RuleKind::Unknown : rs.kind;
        return Schedule::from_ranges(intervals, kind, rule_comment(rs));
    };

    std::optional<Schedule> today =
        build(date, time_selector_intervals_at(ctx, rs.time_selector, date));

    NaiveDate yesterday_date = date.pred();
    std::optional<Schedule> yesterday = build(
        yesterday_date, time_selector_intervals_at_next_day(ctx, rs.time_selector, yesterday_date));

    if (today && yesterday) return today->addition(*yesterday);
    if (today) return today;
    return yesterday;
}

/// Fold all rule sequences into the day's schedule, applying operator/kind
/// precedence (opening_hours.rs:159-210).
template <class L>
    requires Localize<L>
Schedule compute_schedule_at(const OpeningHoursExpression& expr, const Context<L>& ctx,
                             NaiveDate date) {
    if (!(DATE_START_DATE <= date && date < DATE_END_DATE)) return Schedule{};

    bool prev_match = false;
    std::optional<Schedule> prev_eval;

    for (const auto& rs : expr.rules) {
        bool curr_match = day_selector_filter(ctx, rs.day_selector, date);
        std::optional<Schedule> curr_eval = rule_sequence_schedule_at(rs, date, ctx);

        bool new_match;
        std::optional<Schedule> new_eval;

        bool normal_open_unknown =
            rs.op == RuleOperator::Normal &&
            (rs.kind == RuleKind::Open || rs.kind == RuleKind::Unknown);
        bool overlay = rs.op == RuleOperator::Additional ||
                       (rs.op == RuleOperator::Normal && rs.kind == RuleKind::Closed);

        if (normal_open_unknown) {
            // Whole-day REPLACE (unless the current rule doesn't match today).
            new_match = curr_match || prev_match;
            if (curr_match) {
                new_eval = std::move(curr_eval);
            } else if (prev_eval && !prev_eval->is_always_closed_with_no_comments()) {
                new_eval = std::move(prev_eval);
            } else {
                new_eval = std::move(curr_eval);
            }
        } else if (overlay) {
            // Painter OVERLAY (this is what makes "; Su,PH off" overlay).
            new_match = prev_match || curr_match;
            if (prev_eval && curr_eval) {
                new_eval = prev_eval->addition(*curr_eval);
            } else if (prev_eval) {
                new_eval = std::move(prev_eval);
            } else {
                new_eval = std::move(curr_eval);
            }
        } else {
            // Fallback (||): keep prev unless it is absent/degenerate.
            bool prev_degenerate = !prev_eval || prev_eval->is_always_closed_with_no_comments();
            if (prev_match && !prev_degenerate) {
                new_match = prev_match;
                new_eval = std::move(prev_eval);
            } else {
                new_match = curr_match;
                new_eval = std::move(curr_eval);
            }
        }

        prev_match = new_match;
        prev_eval = std::move(new_eval);
    }

    if (prev_eval) return prev_eval->filter_closed_ranges();
    return Schedule{};
}

/// Lower bound on the next date where a different set of rules could match
/// (a performance skip — opening_hours.rs:134-156).
template <class L>
    requires Localize<L>
std::optional<NaiveDate> compute_next_change_hint(const OpeningHoursExpression& expr,
                                                  const Context<L>& ctx, NaiveDate date) {
    if (date < DATE_START_DATE) return DATE_START_DATE;
    if (expr_is_constant(expr)) return DATE_END_DATE;

    std::optional<NaiveDate> best;
    bool first = true;
    for (const auto& rs : expr.rules) {
        std::optional<NaiveDate> h;
        if (time_selector_is_immutable_full_day(rs.time_selector) ||
            !day_selector_filter(ctx, rs.day_selector, date)) {
            h = day_selector_next_change_hint(ctx, rs.day_selector, date);
        } else {
            h = date.succ();
        }
        if (first) {
            best = h;
            first = false;
        } else {
            best = min_opt(best, h);
        }
    }
    return best;
}

/// The lazy interval engine: yields maximal constant-state `DateTimeRange`s in
/// naive local time (opening_hours.rs:437-566).
template <class L>
    requires Localize<L>
class TimeDomainIterator {
    std::shared_ptr<const OpeningHoursExpression> expr_;
    Context<L> ctx_;
    NaiveDate curr_date_;
    ScheduleIter curr_schedule_;
    NaiveDateTime end_datetime_;

    static bool range_contains(const TimeRange& tr, ExtendedTime t) {
        return tr.start <= t && t < tr.end;
    }

  public:
    TimeDomainIterator(std::shared_ptr<const OpeningHoursExpression> expr, Context<L> ctx,
                       NaiveDateTime start_datetime, NaiveDateTime end_datetime)
        : expr_(std::move(expr)), ctx_(std::move(ctx)), curr_date_(start_datetime.date()),
          end_datetime_(end_datetime) {
        ExtendedTime start_time{uint8_t(start_datetime.hour()),
                                uint8_t(start_datetime.minute_of_hour())};
        curr_schedule_ = ScheduleIter(compute_schedule_at(*expr_, ctx_, curr_date_));

        if (start_datetime >= end_datetime) {
            while (curr_schedule_.next()) {
            }
        }

        while (curr_schedule_.peek() && !range_contains(*curr_schedule_.peek(), start_time)) {
            curr_schedule_.next();
        }
    }

    std::optional<DateTimeRange<NaiveDateTime>> next() {
        const TimeRange* peek = curr_schedule_.peek();
        if (!peek) return std::nullopt;

        TimeRange curr_tr = *peek;
        NaiveDateTime start = NaiveDateTime::from_date_time(curr_date_, curr_tr.start);

        consume_until_next_state({curr_tr.kind, curr_tr.comment});
        NaiveDate end_date = curr_date_;

        ExtendedTime end_time = curr_schedule_.peek() ? curr_schedule_.peek()->start
                                                      : ExtendedTime::midnight_00();
        NaiveDateTime end = std::min(end_datetime_, NaiveDateTime::from_date_time(end_date, end_time));

        return DateTimeRange<NaiveDateTime>{start, end, curr_tr.kind, curr_tr.comment};
    }

  private:
    void consume_until_next_state(std::pair<RuleKind, std::string> curr_state) {
        auto peek_state_matches = [&]() {
            const TimeRange* p = curr_schedule_.peek();
            return p && p->kind == curr_state.first && p->comment == curr_state.second;
        };

        while (peek_state_matches()) {
            curr_schedule_.next();

            if (!curr_schedule_.peek()) {
                std::optional<NaiveDate> hint = compute_next_change_hint(*expr_, ctx_, curr_date_);
                NaiveDate next_hint = hint ? *hint : curr_date_.succ();

                // Infinite-loop guard (matches the Rust assert).
                if (!(next_hint > curr_date_)) next_hint = curr_date_.succ();
                curr_date_ = next_hint;

                if (curr_date_ > end_datetime_.date() || DATE_END_DATE <= curr_date_) break;

                curr_schedule_ = ScheduleIter(compute_schedule_at(*expr_, ctx_, curr_date_));
            }
        }
    }
};

// Forward declaration so OpeningHours can name it.
template <class L>
    requires Localize<L>
class OpeningHours;

/// First naive interval in [from, to), clamped — the lazy building block for
/// `state` and `next_change`.
template <class L>
    requires Localize<L>
std::optional<DateTimeRange<NaiveDateTime>>
first_naive_interval(const std::shared_ptr<const OpeningHoursExpression>& expr, const Context<L>& ctx,
                     NaiveDateTime from, NaiveDateTime to) {
    NaiveDateTime f = std::min(DATE_END, from);
    NaiveDateTime t = std::min(DATE_END, to);
    TimeDomainIterator<L> it(expr, ctx, f, t);
    auto dtr = it.next();
    if (!dtr || !(dtr->start < t)) return std::nullopt;
    NaiveDateTime start = std::max(dtr->start, f);
    NaiveDateTime end = std::min(dtr->end, t);
    return DateTimeRange<NaiveDateTime>{start, end, dtr->kind, dtr->comment};
}

/// All naive intervals in [from, to), clamped (materialised).
template <class L>
    requires Localize<L>
std::vector<DateTimeRange<NaiveDateTime>>
collect_naive_range(const std::shared_ptr<const OpeningHoursExpression>& expr, const Context<L>& ctx,
                    NaiveDateTime from, NaiveDateTime to) {
    NaiveDateTime f = std::min(DATE_END, from);
    NaiveDateTime t = std::min(DATE_END, to);
    std::vector<DateTimeRange<NaiveDateTime>> out;
    TimeDomainIterator<L> it(expr, ctx, f, t);
    while (auto dtr = it.next()) {
        if (!(dtr->start < t)) break;
        NaiveDateTime start = std::max(dtr->start, f);
        NaiveDateTime end = std::min(dtr->end, t);
        out.push_back(DateTimeRange<NaiveDateTime>{start, end, dtr->kind, dtr->comment});
    }
    return out;
}

/// A parsed expression + its evaluation context. Cheap to copy (the expression
/// is shared). Generic over the locale `L`.
template <class L = NoLocation>
    requires Localize<L>
class OpeningHours {
    std::shared_ptr<const OpeningHoursExpression> expr_;
    Context<L> ctx_;

  public:
    using DateTime = typename L::DateTime;

    explicit OpeningHours(OpeningHoursExpression expr, Context<L> ctx = {})
        : expr_(std::make_shared<const OpeningHoursExpression>(std::move(expr))),
          ctx_(std::move(ctx)) {}
    OpeningHours(std::shared_ptr<const OpeningHoursExpression> expr, Context<L> ctx)
        : expr_(std::move(expr)), ctx_(std::move(ctx)) {}

    const Context<L>& get_context() const { return ctx_; }
    std::shared_ptr<const OpeningHoursExpression> get_expression() const { return expr_; }
    const OpeningHoursExpression& expression() const { return *expr_; }
    std::string to_string() const { return expr_->to_string(); }

    /// Attach a new context (possibly changing the locale type).
    template <class L2>
        requires Localize<L2>
    OpeningHours<L2> with_context(Context<L2> ctx) const {
        return OpeningHours<L2>(expr_, std::move(ctx));
    }

    Schedule schedule_at(NaiveDate date) const {
        return compute_schedule_at(*expr_, ctx_, date);
    }

    std::optional<NaiveDate> next_change_hint(NaiveDate date) const {
        return compute_next_change_hint(*expr_, ctx_, date);
    }

    /// State (kind, comment) at a given time; defaults to (Closed, "").
    std::pair<RuleKind, std::string> state(DateTime t) const {
        NaiveDateTime from = ctx_.locale.naive(t);
        NaiveDateTime to = ctx_.locale.naive(t.add_minutes(1));
        auto first = first_naive_interval<L>(expr_, ctx_, from, to);
        if (first) return {first->kind, first->comment};
        return {RuleKind::Closed, std::string()};
    }

    bool is_open(DateTime t) const { return state(t).first == RuleKind::Open; }
    bool is_closed(DateTime t) const { return state(t).first == RuleKind::Closed; }
    bool is_unknown(DateTime t) const { return state(t).first == RuleKind::Unknown; }

    /// Next time the state changes, or `nullopt` if the horizon (DATE_END) is
    /// reached first.
    std::optional<DateTime> next_change(DateTime current_time) const {
        NaiveDateTime from = ctx_.locale.naive(current_time);
        NaiveDateTime to = ctx_.locale.naive(ctx_.locale.datetime(DATE_END));
        auto first = first_naive_interval<L>(expr_, ctx_, from, to);
        if (!first) return std::nullopt;
        DateTime end_dt = ctx_.locale.datetime(first->end);
        if (ctx_.locale.naive(end_dt) >= DATE_END) return std::nullopt;
        return end_dt;
    }

    /// Disjoint intervals of differing state in [from, to) (materialised).
    std::vector<DateTimeRange<DateTime>> iter_range(DateTime from, DateTime to) const {
        auto naive = collect_naive_range<L>(expr_, ctx_, ctx_.locale.naive(from), ctx_.locale.naive(to));
        std::vector<DateTimeRange<DateTime>> out;
        out.reserve(naive.size());
        for (auto& dtr : naive) {
            out.push_back(DateTimeRange<DateTime>{ctx_.locale.datetime(dtr.start),
                                                  ctx_.locale.datetime(dtr.end), dtr.kind,
                                                  std::move(dtr.comment)});
        }
        return out;
    }

    /// Like `iter_range` but open-ended (up to DATE_END).
    std::vector<DateTimeRange<DateTime>> iter_from(DateTime from) const {
        return iter_range(from, ctx_.locale.datetime(DATE_END));
    }
};

/// Parse an expression into a default-context `OpeningHours`.
inline std::expected<OpeningHours<NoLocation>, std::string>
parse_opening_hours(std::string_view raw) {
    auto expr = parse(raw);
    if (!expr) return std::unexpected(expr.error());
    return OpeningHours<NoLocation>(std::move(*expr));
}

} // namespace opening_hours
