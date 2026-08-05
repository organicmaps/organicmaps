#pragma once

#include "coding/bit_streams.hpp"
#include "coding/writer.hpp"
#include "opening_hours/opening_hours.hpp"

#include "base/checked_cast.hpp"
#include "base/stl_helpers.hpp"

#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// TODO (@gmoryes)
//  opening_hours does not support such string: 2019 Apr - 2020 May,
//  but 2019 Arp 14 - 2020 May 31 supports.

namespace routing
{
/// \brief This class serialise opening hours from OSM format to some binary data.
/// Each opening hours has one, two, ... rules. Each rule has some data, like: Year, Month,
/// Weekdays etc. (look to |Header::Bits| for supported features). This class has two mods:
///     1) (default) Serialize opening hours only if all rules have same headers. In this case
///        we serialize in such format:
///        <number of rules><header><rule 1>...<rule N>
///     2) (Turned on by |SetSerializeEverything| method) For each rule serialize its' header.
///        Format:
///        <number of rules><header 1><rule 1><header 2><rule 2>...<header N><rule N>
///
/// First format can be used in routing for access:conditional because it has more strong filtering
/// of opening hours and the more trash will be dropped.
/// Second format can be used in opening hours for POI because it support more kinds of opening
/// hours and in case of invalid data we can show raw string and user will able to understand what
/// it's supposed to be.
///
/// This class also drops rules with old year range, like:
///     "2010 - 2011"
///     "2010 Arp 19 - 2014 May 20"
///     etc.
/// In case of mixed data:
///     "2010 Arp 19 - 2014 May 20 ; 2020 Mo-Su"
///     "2010 Arp 19 - 2014 May 20 ; Mo-Su 10:00-15:00"
/// It leaves only fresh rules:
///     "2020 Mo-Su" and "Mo-Su 10:00-15:00"
class OpeningHoursSerDes
{
public:
  class PreparedEncoding
  {
  public:
    /// Appends the payload without byte-aligning the destination writer.
    template <typename Writer>
    void Write(BitWriter<Writer> & writer) const;

    uint64_t GetBitCount() const { return m_bitCount; }

  private:
    friend class OpeningHoursSerDes;

    PreparedEncoding(std::vector<uint8_t> bytes, uint64_t bitCount) : m_bytes(std::move(bytes)), m_bitCount(bitCount) {}

    std::vector<uint8_t> m_bytes;
    uint64_t m_bitCount = 0;
  };

  class Header
  {
  public:
    using HeaderType = uint16_t;
    enum class Bits : HeaderType
    {
      Year = 1U << 0U,
      Month = 1U << 1U,
      MonthDay = 1U << 2U,
      WeekDay = 1U << 3U,
      Hours = 1U << 4U,
      Minutes = 1U << 5U,
      Off = 1U << 6U,
      Holiday = 1U << 7U,

      Max = 1U << 8U
    };

    static inline uint8_t constexpr kBitsSize = 8;
    static_assert(base::Underlying(Bits::Max) == 1U << kBitsSize,
                  "It's seems header's size has been changed. Need to change |kBitsSize| member.");

    static_assert(kBitsSize <= 8, "Header's size is more than 1 byte, be careful about section weight.");

    /// \brief Returns true if feature takes only two values (0 or 1).
    static bool IsBinaryFeature(Header::Bits feature);

    template <typename Writer>
    static void Serialize(BitWriter<Writer> & writer, Header header);
    template <typename Reader>
    static Header Deserialize(BitReader<Reader> & reader);

    static uint8_t GetBitsSize() { return kBitsSize; }

    Header() = default;
    explicit Header(HeaderType header) : m_header(header) {}

    bool operator==(Header const & rhs) const { return m_header == rhs.m_header; }
    bool operator!=(Header const & rhs) const { return !(*this == rhs); }

    void Set(Bits bit) { m_header |= base::Underlying(bit); }
    bool IsSet(Bits bit) const { return m_header & base::Underlying(bit); }

    HeaderType GetValue() const { return m_header; }

  private:
    HeaderType m_header = 0;
  };

  OpeningHoursSerDes();

  static OpeningHoursSerDes ForRouting();

  void Enable(Header::Bits bit);
  bool IsEnabled(Header::Bits bit) const;

  void SetSerializeEverything() { m_serializeEverything = true; }

  /// Returns the exact payload only after all structural and field-width
  /// checks pass. No destination writer is modified on failure.
  [[nodiscard]] std::optional<PreparedEncoding> Prepare(osmoh::OpeningHours const & openingHours);
  [[nodiscard]] std::optional<PreparedEncoding> Prepare(std::string const & openingHoursString);

  template <typename Writer>
  bool Serialize(BitWriter<Writer> & writer, osmoh::OpeningHours const & openingHours);

  template <typename Writer>
  bool Serialize(BitWriter<Writer> & writer, std::string const & openingHoursString);

  template <typename Reader>
  osmoh::OpeningHours Deserialize(BitReader<Reader> & reader);

private:
  inline static auto constexpr kMaxRulesCount = std::numeric_limits<uint8_t>::max();
  inline static uint16_t constexpr kYearBias = 2000;

  bool IsTwentyFourHourRule(osmoh::RuleSequence const & rule) const;

  /// Writes pre-decomposed rules to a temporary encoding. A failure leaves all
  /// destination writers untouched.
  template <typename Writer>
  bool WriteDecomposedRules(BitWriter<Writer> & writer, std::vector<osmoh::RuleSequence> const & decomposedRules);

  /// Decomposes and validates opening hours rules. Returns empty vector on failure.
  std::vector<osmoh::RuleSequence> DecomposeAndValidate(osmoh::OpeningHours const & openingHours);

  /// True for rules that never serialize regardless of content: stale year
  /// ranges and empty rules with a useless modifier.
  bool Skipped(osmoh::RuleSequence const & rule) const;

  /// True when any live rule carries a selector the wire format has no field
  /// for (week selectors, year/month/time periods, event times); such values
  /// are refused whole instead of being stored degraded.
  bool HasUnencodableSelectors(osmoh::TRuleSequences const & rules) const;

  /// True when a later non-additive open rule overlaps an earlier one (or a
  /// rule follows 24/7): the boundary-less wire format cannot keep the
  /// override semantics, so such values are not serialized.
  bool HasOverridingOverlap(osmoh::TRuleSequences const & rules) const;

  /// Weekdays of |rule| that survive DecomposeOh and the NotSupported filter,
  /// i.e. the days its serialized parts actually cover; 0 for a rule that
  /// never reaches the wire.
  uint32_t WireWeekdayMask(osmoh::RuleSequence const & rule) const;

  std::vector<osmoh::RuleSequence> DecomposeOh(osmoh::OpeningHours const & oh);
  Header CreateHeader(osmoh::RuleSequence const & rule) const;
  bool NotSupported(osmoh::RuleSequence const & rule) const;
  bool ExistsFeatureInOpeningHours(Header::Bits feature, osmoh::RuleSequence const & rule) const;
  bool CheckSupportedFeatures() const;
  bool HaveSameHeaders(std::vector<osmoh::RuleSequence> const & decomposedRules) const;

  template <typename Writer>
  bool Serialize(BitWriter<Writer> & writer, Header::Bits type, osmoh::RuleSequence const & rule);

  template <typename Reader>
  void Deserialize(BitReader<Reader> & reader, Header::Bits type, osmoh::RuleSequence & rule);

  template <typename Writer>
  bool SerializeValue(BitWriter<Writer> & writer, Header::Bits type, uint8_t value);

  template <typename Writer>
  void SerializeRuleHeader(BitWriter<Writer> & writer, osmoh::RuleSequence const & rule);

  uint8_t GetBitsNumber(Header::Bits type) const;
  bool CheckYearRange(osmoh::MonthDay::TYear start, osmoh::MonthDay::TYear end) const;

  uint16_t m_currentYear;
  bool m_serializeEverything = false;
  std::vector<Header::Bits> m_supportedFeatures;
  std::vector<Header::Bits> m_unsupportedFeatures;
};

// OpeningHoursSerDes::Header ----------------------------------------------------------------------
template <typename Writer>
void OpeningHoursSerDes::Header::Serialize(BitWriter<Writer> & writer, OpeningHoursSerDes::Header header)
{
  writer.Write(header.GetValue(), Header::GetBitsSize());
}

template <typename Reader>
OpeningHoursSerDes::Header OpeningHoursSerDes::Header::Deserialize(BitReader<Reader> & reader)
{
  return Header(reader.Read(Header::GetBitsSize()));
}

// OpeningHoursSerDes ------------------------------------------------------------------------------
template <typename Writer>
void OpeningHoursSerDes::PreparedEncoding::Write(BitWriter<Writer> & writer) const
{
  auto const fullBytes = m_bitCount / 8;
  for (size_t i = 0; i < fullBytes; ++i)
    writer.Write(m_bytes[i], 8);

  auto const remainingBits = static_cast<uint8_t>(m_bitCount % 8);
  if (remainingBits != 0)
    writer.Write(m_bytes[fullBytes], remainingBits);
}

template <typename Writer>
bool OpeningHoursSerDes::Serialize(BitWriter<Writer> & writer, osmoh::OpeningHours const & openingHours)
{
  auto encoding = Prepare(openingHours);
  if (!encoding)
    return false;

  encoding->Write(writer);
  return true;
}

template <typename Writer>
bool OpeningHoursSerDes::Serialize(BitWriter<Writer> & writer, std::string const & openingHoursString)
{
  auto encoding = Prepare(openingHoursString);
  if (!encoding)
    return false;

  encoding->Write(writer);
  return true;
}

template <typename Writer>
bool OpeningHoursSerDes::WriteDecomposedRules(BitWriter<Writer> & writer,
                                              std::vector<osmoh::RuleSequence> const & decomposedRules)
{
  auto const rulesCount = base::checked_cast<uint8_t>(decomposedRules.size());
  writer.Write(rulesCount /* bits */, 8 /* bitsNumber */);

  if (!m_serializeEverything)
  {
    // We assume that all rules have same headers (checked by |HaveSameHeaders| method).
    SerializeRuleHeader(writer, decomposedRules.back());
  }

  for (auto const & rule : decomposedRules)
  {
    if (m_serializeEverything)
      SerializeRuleHeader(writer, rule);

    for (auto const supportedFeature : m_supportedFeatures)
    {
      if (ExistsFeatureInOpeningHours(supportedFeature, rule))
      {
        if (!Serialize(writer, supportedFeature, rule))
          return false;
      }
    }
  }

  return true;
}

template <typename Reader>
osmoh::OpeningHours OpeningHoursSerDes::Deserialize(BitReader<Reader> & reader)
{
  osmoh::TRuleSequences rules;
  uint8_t const rulesCount = reader.Read(8 /* bitsNumber */);
  Header header;
  if (!m_serializeEverything)
    header = Header::Deserialize(reader);

  for (size_t i = 0; i < rulesCount; ++i)
  {
    if (m_serializeEverything)
      header = Header::Deserialize(reader);

    osmoh::RuleSequence rule;
    for (auto const supportedFeature : m_supportedFeatures)
      if (header.IsSet(supportedFeature))
        Deserialize(reader, supportedFeature, rule);

    rules.emplace_back(std::move(rule));
  }

  // DecomposeOh splits a source rule into single-selector parts (a union): one
  // weekday rule with two time windows becomes two rules. Restore the union
  // with the additive separator; with the default ";" the evaluator would let
  // the parts override each other and keep only the last window on days where
  // several parts match.
  for (size_t i = 0; i + 1 < rules.size(); ++i)
    rules[i].SetAnySeparator(",");

  if (rules.size() == 1 && IsTwentyFourHourRule(rules.back()))
  {
    rules.back().SetTimes({});
    rules.back().SetTwentyFourHours(true /* on */);
  }

  return osmoh::OpeningHours(rules);
}

template <typename Writer>
bool OpeningHoursSerDes::Serialize(BitWriter<Writer> & writer, Header::Bits type, osmoh::RuleSequence const & rule)
{
  uint8_t start = 0;
  uint8_t end = 0;

  switch (type)
  {
  case Header::Bits::Year:
  {
    uint16_t startYear = 0;
    uint16_t endYear = 0;
    if (rule.GetYears().size() == 1)
    {
      startYear = rule.GetYears().back().GetStart();
      endYear = rule.GetYears().back().GetEnd();
    }
    else if (rule.GetMonths().size() == 1)
    {
      startYear = rule.GetMonths().back().GetStart().GetYear();
      endYear = rule.GetMonths().back().GetEnd().GetYear();
    }
    else
    {
      UNREACHABLE();
    }

    if (!CheckYearRange(startYear, endYear))
      return false;

    start = base::checked_cast<uint8_t>(startYear - kYearBias);
    end = base::checked_cast<uint8_t>(endYear - kYearBias);
    break;
  }
  case Header::Bits::Month:
  {
    CHECK_EQUAL(rule.GetMonths().size(), 1, ());
    auto const startMonth = rule.GetMonths().back().GetStart().GetMonth();
    auto const endMonth = rule.GetMonths().back().GetEnd().GetMonth();
    start = base::checked_cast<uint8_t>(base::Underlying(startMonth));
    end = base::checked_cast<uint8_t>(base::Underlying(endMonth));
    break;
  }
  case Header::Bits::MonthDay:
  {
    CHECK_EQUAL(rule.GetMonths().size(), 1, ());
    start = rule.GetMonths().back().GetStart().GetDayNum();
    end = rule.GetMonths().back().GetEnd().GetDayNum();
    break;
  }
  case Header::Bits::WeekDay:
  {
    CHECK_EQUAL(rule.GetWeekdays().GetWeekdayRanges().size(), 1, ());
    auto const startWeekday = rule.GetWeekdays().GetWeekdayRanges().back().GetStart();
    auto const endWeekday = rule.GetWeekdays().GetWeekdayRanges().back().GetEnd();
    start = base::checked_cast<uint8_t>(base::Underlying(startWeekday));
    end = base::checked_cast<uint8_t>(base::Underlying(endWeekday));
    break;
  }
  case Header::Bits::Hours:
  {
    CHECK(rule.GetTimes().size() == 1 || rule.IsTwentyFourHours(), ());
    if (rule.IsTwentyFourHours())
    {
      start = 0;
      end = 24;
    }
    else
    {
      start = rule.GetTimes().back().GetStart().GetHoursCount();
      end = rule.GetTimes().back().GetEnd().GetHoursCount();
    }
    break;
  }
  case Header::Bits::Minutes:
  {
    CHECK(rule.GetTimes().size() == 1 || rule.IsTwentyFourHours(), ());
    if (rule.IsTwentyFourHours())
      break;
    start = rule.GetTimes().back().GetStart().GetMinutesCount();
    end = rule.GetTimes().back().GetEnd().GetMinutesCount();
    break;
  }
  case Header::Bits::Off:
  {
    start = rule.GetModifier() == osmoh::RuleSequence::Modifier::Closed;
    end = 0;
    break;
  }
  case Header::Bits::Holiday:
  {
    CHECK_EQUAL(rule.GetWeekdays().GetHolidays().size(), 1, ());
    start = rule.GetWeekdays().GetHolidays().back().IsPlural();
    end = 0;
    break;
  }
  case Header::Bits::Max: UNREACHABLE();
  }

  if (!SerializeValue(writer, type, start))
    return false;

  if (!Header::IsBinaryFeature(type))
    return SerializeValue(writer, type, end);

  return true;
}

template <typename Reader>
void OpeningHoursSerDes::Deserialize(BitReader<Reader> & reader, OpeningHoursSerDes::Header::Bits type,
                                     osmoh::RuleSequence & rule)
{
  uint8_t const start = reader.Read(GetBitsNumber(type));
  uint8_t end = 0;
  if (!Header::IsBinaryFeature(type))
    end = reader.Read(GetBitsNumber(type));

  switch (type)
  {
  case Header::Bits::Year:
  {
    osmoh::YearRange range;
    range.SetStart(start + kYearBias);
    range.SetEnd(end + kYearBias);
    rule.SetYears({range});
    break;
  }
  case Header::Bits::Month:
  {
    osmoh::MonthDay startMonth;
    startMonth.SetMonth(static_cast<osmoh::MonthDay::Month>(start));

    osmoh::MonthDay endMonth;
    endMonth.SetMonth(static_cast<osmoh::MonthDay::Month>(end));

    if (rule.HasYears())
    {
      CHECK_EQUAL(rule.GetYears().size(), 1, ());
      startMonth.SetYear(rule.GetYears().back().GetStart());
      endMonth.SetYear(rule.GetYears().back().GetEnd());
      rule.SetYears({});
    }

    osmoh::MonthdayRange range;
    range.SetStart(startMonth);
    range.SetEnd(endMonth);
    rule.SetMonths({range});
    break;
  }
  case Header::Bits::MonthDay:
  {
    CHECK_EQUAL(rule.GetMonths().size(), 1, ());
    auto range = rule.GetMonths().back();

    auto startMonth = range.GetStart();
    auto endMonth = range.GetEnd();

    startMonth.SetDayNum(start);
    endMonth.SetDayNum(end);

    range.SetStart(startMonth);
    range.SetEnd(endMonth);

    rule.SetMonths({range});
    break;
  }
  case Header::Bits::WeekDay:
  {
    osmoh::WeekdayRange range;

    auto const startWeekday = static_cast<osmoh::Weekday>(start);
    range.SetStart(startWeekday);

    auto const endWeekday = static_cast<osmoh::Weekday>(end);
    range.SetEnd(endWeekday);

    osmoh::Weekdays weekdays;
    weekdays.SetWeekdayRanges({range});
    rule.SetWeekdays(weekdays);
    break;
  }
  case Header::Bits::Hours:
  {
    osmoh::Timespan range;
    auto hm = range.GetStart().GetHourMinutes();
    hm.SetHours(osmoh::HourMinutes::THours(start));
    range.GetStart().SetHourMinutes(hm);

    hm = range.GetEnd().GetHourMinutes();
    hm.SetHours(osmoh::HourMinutes::THours(end));
    range.GetEnd().SetHourMinutes(hm);

    rule.SetTimes({range});
    break;
  }
  case Header::Bits::Minutes:
  {
    CHECK_EQUAL(rule.GetTimes().size(), 1, ());
    osmoh::Timespan range = rule.GetTimes().back();

    range.GetStart().GetHourMinutes().SetMinutes(osmoh::HourMinutes::TMinutes(start));
    range.GetEnd().GetHourMinutes().SetMinutes(osmoh::HourMinutes::TMinutes(end));
    rule.SetTimes({range});
    break;
  }
  case Header::Bits::Off:
  {
    CHECK(start == 0 || start == 1, (start));
    CHECK_EQUAL(end, 0, ());
    if (start)
      rule.SetModifier(osmoh::RuleSequence::Modifier::Closed);
    break;
  }
  case Header::Bits::Holiday:
  {
    CHECK(start == 0 || start == 1, (start));
    CHECK_EQUAL(end, 0, ());
    auto const plural = static_cast<bool>(start);

    osmoh::Holiday holiday;
    holiday.SetPlural(plural);

    osmoh::Weekdays weekdays;
    weekdays.SetHolidays({holiday});

    rule.SetWeekdays({weekdays});
    break;
  }
  case Header::Bits::Max: UNREACHABLE();
  }
}

template <typename Writer>
bool OpeningHoursSerDes::SerializeValue(BitWriter<Writer> & writer, Header::Bits type, uint8_t value)
{
  uint8_t const bitsNumber = GetBitsNumber(type);
  if (value >= 1U << bitsNumber)
    return false;

  writer.Write(value, bitsNumber);
  return true;
}

template <typename Writer>
void OpeningHoursSerDes::SerializeRuleHeader(BitWriter<Writer> & writer, osmoh::RuleSequence const & rule)
{
  Header const header = CreateHeader(rule);
  Header::Serialize(writer, header);
}
}  // namespace routing
