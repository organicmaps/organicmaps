#include "routing/opening_hours_serdes.hpp"

#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

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
    return true; // Wrong data. |startYear| is later than |endYear|.

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

bool OpeningHoursSerDes::NotSupported(osmoh::RuleSequence const & rule)
{
  for (auto const unsupportedFeature : m_unsupportedFeatures)
  {
    if (ExistsFeatureInOpeningHours(unsupportedFeature, rule))
      return true;
  }

  return false;
}

bool OpeningHoursSerDes::ExistsFeatureInOpeningHours(Header::Bits feature,
                                                     osmoh::RuleSequence const & rule) const
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
    {
      if (monthRange.GetStart().HasYear() && monthRange.GetEnd().HasYear())
        return true;
    }

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
  auto const apply = [&](auto & rules, auto const & ranges, auto const & rangeSetter) {
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
    apply(rules, rule.GetYears(),
          [&](osmoh::YearRange const & range, osmoh::RuleSequence & item) {
            if (ShouldSkipYear(range, m_currentYear))
              badRule = true;

            item.SetYears({range});
          });

    apply(rules, rule.GetMonths(),
          [&](osmoh::MonthdayRange const & range, osmoh::RuleSequence & item) {
            if (ShouldSkipYear(range, m_currentYear))
              badRule = true;

            item.SetMonths({range});
          });

    if (badRule)
      continue;

    apply(rules, rule.GetWeekdays().GetWeekdayRanges(),
          [](osmoh::WeekdayRange const & range, osmoh::RuleSequence & item) {
            osmoh::Weekdays weekdays;
            weekdays.SetWeekdayRanges({range});
            item.SetWeekdays(weekdays);
          });

    apply(rules, rule.GetWeekdays().GetHolidays(),
          [](osmoh::Holiday const & holiday, osmoh::RuleSequence & item) {
            auto weekdays = item.GetWeekdays();
            weekdays.SetHolidays({holiday});
            item.SetWeekdays(weekdays);
          });

    apply(rules, rule.GetTimes(),
          [](osmoh::Timespan const & range, osmoh::RuleSequence & item) {
            item.SetTimes({range});
          });

    apply(rules, std::vector<osmoh::RuleSequence::Modifier>{rule.GetModifier()},
          [](osmoh::RuleSequence::Modifier modifier, osmoh::RuleSequence & item) {
            item.SetModifier(modifier);
          });

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

bool OpeningHoursSerDes::HaveSameHeaders(
    std::vector<osmoh::RuleSequence> const & decomposedOhs) const
{
  CHECK(!decomposedOhs.empty(), ());
  Header const header = CreateHeader(decomposedOhs.front());
  for (auto const & oh : decomposedOhs)
  {
    if (header != CreateHeader(oh))
      return false;
  }

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

bool OpeningHoursSerDes::CheckYearRange(osmoh::MonthDay::TYear start,
                                        osmoh::MonthDay::TYear end) const
{
  if (start < kYearBias || end < kYearBias)
    return false;

  // Should be filtered after |DecomposeOh| method.
  CHECK(start <= end && end >= m_currentYear, (start, end, m_currentYear));
  return true;
}

bool OpeningHoursSerDes::IsTwentyFourHourRule(osmoh::RuleSequence const & rule) const
{
  static auto const kTwentyFourHourStart = osmoh::HourMinutes(osmoh::HourMinutes::THours(0));
  static auto const kTwentyFourHourEnd = osmoh::HourMinutes(osmoh::HourMinutes::THours(24));

  return rule.GetModifier() != osmoh::RuleSequence::Modifier::Closed &&
         rule.GetYears().empty() && rule.GetWeekdays().GetWeekdayRanges().empty() &&
         rule.GetMonths().empty() && rule.GetTimes().size() == 1 &&
         rule.GetTimes().back().GetStart() == kTwentyFourHourStart &&
         rule.GetTimes().back().GetEnd() == kTwentyFourHourEnd;
}


bool OpeningHoursSerDes::IsSerializable(osmoh::OpeningHours const & openingHours)
{
  std::vector<uint8_t> buffer;
  MemWriter memWriter(buffer);
  BitWriter tmpBitWriter(memWriter);

  return SerializeImpl(tmpBitWriter, openingHours);
}
}  // namespace routing
