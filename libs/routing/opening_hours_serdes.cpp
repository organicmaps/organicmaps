#include "routing/opening_hours_serdes.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <bitset>
#include <chrono>

namespace
{
uint16_t GetCurrentYear()
{
  auto const now = std::chrono::system_clock::now();
  std::time_t nowCType = std::chrono::system_clock::to_time_t(now);
  auto parts = std::localtime(&nowCType);
  return 1900 + parts->tm_year;
}

osmoh::RuleSequence GetTwentyFourHourRule()
{
  osmoh::Timespan range;

  auto hm = range.GetStart().GetHourMinutes();
  hm.SetHours(osmoh::HourMinutes::THours(0));
  range.GetStart().SetHourMinutes(hm);

  hm = range.GetStart().GetHourMinutes();
  hm.SetHours(osmoh::HourMinutes::THours(24));
  range.GetEnd().SetHourMinutes(hm);

  osmoh::RuleSequence result;
  result.SetTimes({range});
  return result;
}

bool ShouldSkipYear(uint16_t startYear, uint16_t endYear, uint16_t currentYear)
{
  if (startYear > endYear)
    return true;  // Wrong data. |startYear| is later than |endYear|.

  // returns true if |startYear| and |endYear| are too old and false otherwise.
  return endYear < currentYear;
}

bool ShouldSkipYear(osmoh::YearRange const & range, uint16_t currentYear)
{
  return ShouldSkipYear(range.GetStart(), range.GetEnd(), currentYear);
}

bool ShouldSkipYear(osmoh::MonthdayRange const & range, uint16_t currentYear)
{
  auto const hasYear = range.GetStart().HasYear() && range.GetEnd().HasYear();
  if (!hasYear)
    return false;

  return ShouldSkipYear(range.GetStart().GetYear(), range.GetEnd().GetYear(), currentYear);
}

bool UselessModifier(osmoh::RuleSequence const & rule)
{
  return rule.GetModifier() != osmoh::RuleSequence::Modifier::Closed &&
         rule.GetModifier() != osmoh::RuleSequence::Modifier::Open;
}

// Bits [start, end] of a 1-based cyclic range of |count| values ("Fr-Mo",
// "Nov-Mar" wrap around). Out-of-range input yields every bit, so callers
// conservatively treat the rule as matching anything.
uint32_t CyclicMask(int start, int end, int count)
{
  uint32_t const all = (1U << count) - 1;
  if (start < 1 || start > count || end < 1 || end > count)
    return all;

  uint32_t mask = 0;
  for (int i = start;; i = i % count + 1)
  {
    mask |= 1U << (i - 1);
    if (i == end)
      break;
  }
  return mask;
}

uint32_t constexpr kAllWeekdaysMask = 0x7F;

// Days of the year the rule can apply to, on a 12 x 31 grid; no month
// selector means every day. Day numbers matter: "Mar 16-Nov 30" and
// "Dec 01-Mar 15" are disjoint although both touch March.
using DateMask = std::bitset<12 * 31>;

DateMask RuleDateMask(osmoh::RuleSequence const & rule)
{
  DateMask mask;
  if (rule.GetMonths().empty())
    return mask.set();

  for (auto const & range : rule.GetMonths())
  {
    auto const & start = range.GetStart();
    auto const & end = range.HasEnd() ? range.GetEnd() : range.GetStart();
    int const startMonth = static_cast<int>(start.GetMonth());  // Jan = 1.
    int const endMonth = static_cast<int>(end.GetMonth());

    // Variable dates (easter) and malformed months conservatively match
    // anything, mirroring CyclicMask.
    if (start.IsVariable() || end.IsVariable() || startMonth < 1 || startMonth > 12 || endMonth < 1 || endMonth > 12)
      return mask.set();

    int const startDay = start.HasDayNum() ? start.GetDayNum() : 1;
    int const endDay = end.HasDayNum() ? end.GetDayNum() : 31;
    if (startDay > 31 || endDay > 31 || (startMonth == endMonth && startDay > endDay) /* wraps a whole year */)
      return mask.set();

    for (int month = startMonth;; month = month % 12 + 1)
    {
      for (int day = (month == startMonth ? startDay : 1); day <= (month == endMonth ? endDay : 31); ++day)
        mask.set((month - 1) * 31 + (day - 1));
      if (month == endMonth)
        break;
    }
  }
  return mask;
}
}  // namespace

namespace routing
{
// OpeningHoursSerDes::Header ----------------------------------------------------------------------
bool OpeningHoursSerDes::Header::IsBinaryFeature(OpeningHoursSerDes::Header::Bits feature)
{
  switch (feature)
  {
  case Header::Bits::Off:
  case Header::Bits::Holiday: return true;
  default: return false;
  }
  UNREACHABLE();
}

// OpeningHoursSerDes ------------------------------------------------------------------------------
OpeningHoursSerDes::OpeningHoursSerDes() : m_currentYear(GetCurrentYear())
{
  uint32_t bit = 1;
  while (static_cast<Header::Bits>(bit) != Header::Bits::Max)
  {
    m_unsupportedFeatures.emplace_back(static_cast<Header::Bits>(bit));
    bit <<= 1U;
  }
}

OpeningHoursSerDes OpeningHoursSerDes::ForRouting()
{
  OpeningHoursSerDes res;
  res.Enable(OpeningHoursSerDes::Header::Bits::Year);
  res.Enable(OpeningHoursSerDes::Header::Bits::Month);
  res.Enable(OpeningHoursSerDes::Header::Bits::MonthDay);
  res.Enable(OpeningHoursSerDes::Header::Bits::WeekDay);
  res.Enable(OpeningHoursSerDes::Header::Bits::Hours);
  res.Enable(OpeningHoursSerDes::Header::Bits::Minutes);
  return res;
}

std::optional<OpeningHoursSerDes::PreparedEncoding> OpeningHoursSerDes::Prepare(
    osmoh::OpeningHours const & openingHours)
{
  auto const decomposedRules = DecomposeAndValidate(openingHours);
  if (decomposedRules.empty())
    return std::nullopt;

  std::vector<uint8_t> bytes;
  uint64_t bitCount = 0;
  {
    MemWriter memWriter(bytes);
    BitWriter bitWriter(memWriter);
    if (!WriteDecomposedRules(bitWriter, decomposedRules))
      return std::nullopt;
    bitCount = bitWriter.BitsWritten();
  }
  return PreparedEncoding(std::move(bytes), bitCount);
}

std::optional<OpeningHoursSerDes::PreparedEncoding> OpeningHoursSerDes::Prepare(std::string const & openingHoursString)
{
  osmoh::OpeningHours const openingHours(openingHoursString);
  if (!openingHours.IsValid())
    return std::nullopt;

  return Prepare(openingHours);
}

void OpeningHoursSerDes::Enable(OpeningHoursSerDes::Header::Bits bit)
{
  m_supportedFeatures.emplace_back(bit);
  auto const it = std::find(m_unsupportedFeatures.cbegin(), m_unsupportedFeatures.cend(), bit);
  if (it != m_unsupportedFeatures.cend())
    m_unsupportedFeatures.erase(it);
  base::SortUnique(m_supportedFeatures);
}

bool OpeningHoursSerDes::IsEnabled(OpeningHoursSerDes::Header::Bits bit) const
{
  bool const sup = base::IsExist(m_supportedFeatures, bit);
  bool const unsup = base::IsExist(m_unsupportedFeatures, bit);

  CHECK_NOT_EQUAL(sup, unsup, ());
  return sup;
}

OpeningHoursSerDes::Header OpeningHoursSerDes::CreateHeader(osmoh::RuleSequence const & rule) const
{
  Header header;
  for (auto const & supportedFeature : m_supportedFeatures)
  {
    if (ExistsFeatureInOpeningHours(supportedFeature, rule))
    {
      header.Set(supportedFeature);
      // We always store minutes if we have hours and vice versa. We divided them to have more
      // atomic serialization and deserialization.
      if (supportedFeature == Header::Bits::Hours)
        header.Set(Header::Bits::Minutes);
      if (supportedFeature == Header::Bits::Minutes)
        header.Set(Header::Bits::Hours);
    }
  }

  return header;
}

bool OpeningHoursSerDes::NotSupported(osmoh::RuleSequence const & rule) const
{
  for (auto const unsupportedFeature : m_unsupportedFeatures)
    if (ExistsFeatureInOpeningHours(unsupportedFeature, rule))
      return true;

  // The WeekDay field stores only a plain range: nth entries ("Mo[1]") and
  // day offsets would silently serialize as every such weekday. The Holiday
  // bit likewise stores no offset ("PH -1 day").
  for (auto const & range : rule.GetWeekdays().GetWeekdayRanges())
    if (range.HasNth() || range.GetOffset() != 0)
      return true;
  for (auto const & holiday : rule.GetWeekdays().GetHolidays())
    if (holiday.GetOffset() != 0)
      return true;

  // The MonthDay field stores a plain year/month/day only: "easter" would come
  // back as "Jan 01", "Mar 15 +Sa" as plain "Mar 15", and "Mar Su[-1]" as the
  // whole of March -- a restriction up to a month too wide.
  for (auto const & range : rule.GetMonths())
  {
    auto const lossy = [](osmoh::MonthDay const & md)
    { return md.IsVariable() || md.HasOffset() || md.HasWeekdayInMonth(); };
    if (lossy(range.GetStart()) || (range.HasEnd() && lossy(range.GetEnd())))
      return true;
  }

  return false;
}

bool OpeningHoursSerDes::ExistsFeatureInOpeningHours(Header::Bits feature, osmoh::RuleSequence const & rule) const
{
  switch (feature)
  {
  case Header::Bits::Year:
  {
    // 2019-2020 it is just |rule.HasYears()|
    // 2019 Apr - 2020 May - it is year in |osmoh::MonthDay| and |rule.HasYear()| == false.
    if (rule.HasYears())
      return true;

    for (auto const & monthRange : rule.GetMonths())
      if (monthRange.GetStart().HasYear() && monthRange.GetEnd().HasYear())
        return true;

    return false;
  }
  case Header::Bits::Month: return rule.HasMonths();
  case Header::Bits::MonthDay: return rule.HasMonthDay();
  case Header::Bits::WeekDay: return rule.GetWeekdays().HasWeekday();
  case Header::Bits::Hours:
  case Header::Bits::Minutes: return rule.HasTimes() || rule.IsTwentyFourHours();
  case Header::Bits::Off: return rule.GetModifier() == osmoh::RuleSequence::Modifier::Closed;
  case Header::Bits::Holiday: return rule.GetWeekdays().HasHolidays();
  case Header::Bits::Max: CHECK(false, ());
  }
  UNREACHABLE();
}

bool OpeningHoursSerDes::CheckSupportedFeatures() const
{
  if (IsEnabled(Header::Bits::MonthDay) && !IsEnabled(Header::Bits::Month))
    CHECK(false, ("Cannot use MonthDay without Month."));

  if (IsEnabled(Header::Bits::Hours) != IsEnabled(Header::Bits::Minutes))
    CHECK(false, ("Cannot use Hours without Minutes and vice versa."));

  return true;
}

std::vector<osmoh::RuleSequence> OpeningHoursSerDes::DecomposeOh(osmoh::OpeningHours const & oh)
{
  auto const apply = [&](auto & rules, auto const & ranges, auto const & rangeSetter)
  {
    if (ranges.empty())
      return;

    std::vector<osmoh::RuleSequence> originalRules = std::move(rules);
    if (originalRules.empty())
      originalRules.emplace_back();

    rules.clear();
    for (auto const & range : ranges)
    {
      auto const toDo = [&range, &rangeSetter](auto & rule) { rangeSetter(range, rule); };
      for (auto const & originalRule : originalRules)
      {
        auto rule = originalRule;
        toDo(rule);
        rules.emplace_back(std::move(rule));
      }
    }
  };

  std::vector<osmoh::RuleSequence> finalRules;
  for (auto const & rule : oh.GetRule())
  {
    std::vector<osmoh::RuleSequence> rules;

    if (rule.IsTwentyFourHours())
    {
      static auto const kTwentyFourHourRule = GetTwentyFourHourRule();
      finalRules.emplace_back(kTwentyFourHourRule);
      break;
    }

    // Ir rule has just modifier (opening hours: "closed") it is empty but has a useful modifier.
    if (rule.IsEmpty() && UselessModifier(rule))
      continue;

    bool badRule = false;
    apply(rules, rule.GetYears(), [&](osmoh::YearRange const & range, osmoh::RuleSequence & item)
    {
      if (ShouldSkipYear(range, m_currentYear))
        badRule = true;

      item.SetYears({range});
    });

    apply(rules, rule.GetMonths(), [&](osmoh::MonthdayRange const & range, osmoh::RuleSequence & item)
    {
      if (ShouldSkipYear(range, m_currentYear))
        badRule = true;

      item.SetMonths({range});
    });

    if (badRule)
      continue;

    // Weekday ranges and holidays are alternatives (a union): each spawns its
    // own single-selector rule. Overlaying a holiday onto a weekday rule would
    // make routing, which does not support the Holiday bit, drop the weekday
    // part together with the holiday in NotSupported(). The wire is
    // self-describing through per-rule header bits. ForRouting() omits the
    // Holiday bit, so holiday-only rules are filtered while weekday rules stay.
    std::vector<osmoh::Weekdays> daySelectors;
    for (auto const & range : rule.GetWeekdays().GetWeekdayRanges())
    {
      osmoh::Weekdays wd;
      wd.SetWeekdayRanges({range});
      daySelectors.push_back(std::move(wd));
    }
    for (auto const & holiday : rule.GetWeekdays().GetHolidays())
    {
      osmoh::Weekdays wd;
      wd.SetHolidays({holiday});
      daySelectors.push_back(std::move(wd));
    }
    apply(rules, daySelectors,
          [](osmoh::Weekdays const & weekdays, osmoh::RuleSequence & item) { item.SetWeekdays(weekdays); });

    apply(rules, rule.GetTimes(),
          [](osmoh::Timespan const & range, osmoh::RuleSequence & item) { item.SetTimes({range}); });

    apply(rules, std::vector<osmoh::RuleSequence::Modifier>{rule.GetModifier()},
          [](osmoh::RuleSequence::Modifier modifier, osmoh::RuleSequence & item) { item.SetModifier(modifier); });

    finalRules.insert(finalRules.end(), rules.begin(), rules.end());
  }

  std::vector<osmoh::RuleSequence> filteredRules;
  for (auto rule : finalRules)
  {
    if (NotSupported(rule))
      continue;

    filteredRules.emplace_back(std::move(rule));
  }

  return filteredRules;
}

bool OpeningHoursSerDes::HaveSameHeaders(std::vector<osmoh::RuleSequence> const & decomposedOhs) const
{
  CHECK(!decomposedOhs.empty(), ());
  Header const header = CreateHeader(decomposedOhs.front());
  for (auto const & oh : decomposedOhs)
    if (header != CreateHeader(oh))
      return false;

  return true;
}

uint8_t OpeningHoursSerDes::GetBitsNumber(Header::Bits type) const
{
  switch (type)
  {
  case Header::Bits::Year: return 8;  // store value such that 2000 + value equals to real year
  case Header::Bits::Month: return 4;
  case Header::Bits::MonthDay: return 5;
  case Header::Bits::WeekDay: return 3;
  case Header::Bits::Hours: return 5;
  case Header::Bits::Minutes: return 6;
  case Header::Bits::Off: return 1;
  case Header::Bits::Holiday: return 1;
  case Header::Bits::Max: UNREACHABLE();
  }
  UNREACHABLE();
}

bool OpeningHoursSerDes::CheckYearRange(osmoh::MonthDay::TYear start, osmoh::MonthDay::TYear end) const
{
  if (start < kYearBias || end < kYearBias)
    return false;

  // The wire stores year - kYearBias in 8 bits. Open-ended selectors ("2020+")
  // materialize as end year 9999, which must refuse gracefully, not overflow.
  if (end - kYearBias > 255)
    return false;

  // Should be filtered after |DecomposeOh| method.
  CHECK(start <= end && end >= m_currentYear, (start, end, m_currentYear));
  return true;
}

bool OpeningHoursSerDes::IsTwentyFourHourRule(osmoh::RuleSequence const & rule) const
{
  static auto const kTwentyFourHourStart = osmoh::HourMinutes(osmoh::HourMinutes::THours(0));
  static auto const kTwentyFourHourEnd = osmoh::HourMinutes(osmoh::HourMinutes::THours(24));

  return rule.GetModifier() != osmoh::RuleSequence::Modifier::Closed && rule.GetYears().empty() &&
         rule.GetWeekdays().GetWeekdayRanges().empty() && rule.GetMonths().empty() && rule.GetTimes().size() == 1 &&
         rule.GetTimes().back().GetStart() == kTwentyFourHourStart &&
         rule.GetTimes().back().GetEnd() == kTwentyFourHourEnd;
}

uint32_t OpeningHoursSerDes::WireWeekdayMask(osmoh::RuleSequence const & rule) const
{
  // Every decomposed part inherits the rule's year/month/time/modifier
  // selectors, so an unsupported one among them kills the whole rule.
  for (auto const feature : m_unsupportedFeatures)
    if (feature != Header::Bits::WeekDay && feature != Header::Bits::Holiday &&
        ExistsFeatureInOpeningHours(feature, rule))
      return 0;

  auto const & weekdays = rule.GetWeekdays();
  if (weekdays.GetWeekdayRanges().empty() && weekdays.GetHolidays().empty())
    return kAllWeekdaysMask;

  // Day selectors decompose into one part per weekday range / holiday, and
  // NotSupported() then drops the nth/offset and unsupported ones, so only the
  // plain ranges reach the wire ("Mo-Fr,PH ..." serializes its Mo-Fr part).
  uint32_t mask = 0;
  if (IsEnabled(Header::Bits::WeekDay))
  {
    for (auto const & range : weekdays.GetWeekdayRanges())
    {
      if (range.HasNth() || range.GetOffset() != 0)
        continue;
      int const start = static_cast<int>(range.GetStart());  // Sunday = 1 .. Saturday = 7.
      mask |= CyclicMask(start, range.HasEnd() ? static_cast<int>(range.GetEnd()) : start, 7);
    }
  }

  // A surviving holiday part can fall on any weekday.
  if (IsEnabled(Header::Bits::Holiday))
    for (auto const & holiday : weekdays.GetHolidays())
      if (holiday.GetOffset() == 0)
        return kAllWeekdaysMask;

  return mask;
}

bool OpeningHoursSerDes::Skipped(osmoh::RuleSequence const & rule) const
{
  // A 24/7 rule has no selectors but is meaningful (DecomposeOh handles it
  // before its emptiness filter).
  if (rule.IsTwentyFourHours())
    return false;
  if (rule.IsEmpty() && UselessModifier(rule))
    return true;
  for (auto const & range : rule.GetYears())
    if (ShouldSkipYear(range, m_currentYear))
      return true;
  for (auto const & range : rule.GetMonths())
    if (ShouldSkipYear(range, m_currentYear))
      return true;
  return false;
}

bool OpeningHoursSerDes::HasUnencodableSelectors(osmoh::TRuleSequences const & rules) const
{
  for (auto const & rule : rules)
  {
    if (Skipped(rule))
      continue;

    // Week selectors are not serialized at all, periods have no wire field,
    // and event times serialize as their 00:00 placeholder: the stored value
    // would be plainly wrong ("week 27" active every week, "sunrise-sunset"
    // open around the clock). Refuse the whole value because serializing a
    // subset changes the condition.
    if (!rule.GetWeeks().empty())
      return true;
    for (auto const & range : rule.GetYears())
      if (range.HasPeriod())
        return true;
    for (auto const & range : rule.GetMonths())
    {
      if (range.HasPeriod())
        return true;
      // The Year field is shared by the two MonthDay endpoints.
      bool const startHasYear = range.GetStart().HasYear();
      bool const endHasYear = range.HasEnd() && range.GetEnd().HasYear();
      if (startHasYear != endHasYear)
        return true;
    }
    for (auto const & span : rule.GetTimes())
      if (span.HasPeriod() || span.GetStart().IsEvent() || (span.HasEnd() && span.GetEnd().IsEvent()))
        return true;
  }
  return false;
}

bool OpeningHoursSerDes::HasOverridingOverlap(osmoh::TRuleSequences const & rules) const
{
  bool seenTwentyFourSeven = false;
  for (size_t j = 0; j < rules.size(); ++j)
  {
    if (Skipped(rules[j]))
      continue;

    // DecomposeOh keeps only the 24/7 rule and discards everything after it,
    // so any later meaningful rule would be lost: refuse. This covers rules
    // the serializer drops too -- losing them is exactly the loss to catch.
    if (seenTwentyFourSeven)
      return true;
    if (rules[j].IsTwentyFourHours())
    {
      seenTwentyFourSeven = true;
      continue;
    }

    uint32_t const overriderMask = WireWeekdayMask(rules[j]);
    if (j == 0 || overriderMask == 0)
      continue;

    // A later closed rule is safe: overlaying "closed" equals overriding.
    // The separator preceding rule j is stored on rule j-1.
    bool const additive = rules[j - 1].GetAnySeparator() == ",";
    bool const overridingOpen = !additive && rules[j].GetModifier() != osmoh::RuleSequence::Modifier::Closed;
    if (!overridingOpen)
      continue;

    for (size_t i = 0; i < j; ++i)
    {
      if (Skipped(rules[i]))
        continue;
      if ((WireWeekdayMask(rules[i]) & overriderMask) != 0 && (RuleDateMask(rules[i]) & RuleDateMask(rules[j])).any())
        return true;
    }
  }
  return false;
}

std::vector<osmoh::RuleSequence> OpeningHoursSerDes::DecomposeAndValidate(osmoh::OpeningHours const & openingHours)
{
  CheckSupportedFeatures();

  if (HasUnencodableSelectors(openingHours.GetRule()))
    return {};

  // The wire format stores no source-rule boundaries and Deserialize joins the
  // decomposed parts back as one union ("," semantics). That is exact for a
  // single source rule and for ";"-rules over disjoint days, but an overriding
  // open rule that overlaps an earlier one would be widened by the union:
  // "Mo-Fr 10:00-18:00; We 12:00-14:00" keeps Wednesday 11:00 closed in the
  // source and would come back open. Refuse to serialize those.
  if (HasOverridingOverlap(openingHours.GetRule()))
    return {};

  auto decomposedRules = DecomposeOh(openingHours);
  if (decomposedRules.empty())
    return {};

  if (!m_serializeEverything && !HaveSameHeaders(decomposedRules))
    return {};

  if (decomposedRules.size() > kMaxRulesCount)
    return {};

  return decomposedRules;
}
}  // namespace routing
