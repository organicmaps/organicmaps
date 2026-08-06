#include "testing/testing.hpp"

#include "routing/opening_hours_serdes.hpp"

#include "platform/platform_tests_support/helpers.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <ctime>
#include <memory>
#include <string>
#include <vector>

namespace opening_hours_serdes_tests
{
using namespace routing;
using namespace platform::tests_support;

using Buffer = std::vector<uint8_t>;

struct OHSerDesTestFixture
{
  OHSerDesTestFixture()
    : m_memWriter(m_buffer)
    , m_bitWriter(std::make_unique<BitWriter<MemWriter<Buffer>>>(m_memWriter))
  {}

  BitWriter<MemWriter<Buffer>> & GetWriter() { return *m_bitWriter; }
  BitReader<ReaderSource<MemReader>> & GetReader()
  {
    if (m_bitReader)
      return *m_bitReader;

    m_memReader = std::make_unique<MemReader>(m_buffer.data(), m_buffer.size());
    m_readerSource = std::make_unique<ReaderSource<MemReader>>(*m_memReader);
    m_bitReader = std::make_unique<BitReader<ReaderSource<MemReader>>>(*m_readerSource);
    return *m_bitReader;
  }

  void Enable(OpeningHoursSerDes::Header::Bits feature) { m_serializer.Enable(feature); }

  void EnableAll()
  {
    Enable(OpeningHoursSerDes::Header::Bits::Year);
    Enable(OpeningHoursSerDes::Header::Bits::Month);
    Enable(OpeningHoursSerDes::Header::Bits::MonthDay);
    Enable(OpeningHoursSerDes::Header::Bits::WeekDay);
    Enable(OpeningHoursSerDes::Header::Bits::Hours);
    Enable(OpeningHoursSerDes::Header::Bits::Minutes);
    Enable(OpeningHoursSerDes::Header::Bits::Off);
    Enable(OpeningHoursSerDes::Header::Bits::Holiday);
  }

  void EnableForRouting() { m_serializer = OpeningHoursSerDes::ForRouting(); }

  bool IsEnabled(OpeningHoursSerDes::Header::Bits feature) { return m_serializer.IsEnabled(feature); }
  bool Serialize(std::string const & oh) { return m_serializer.Serialize(GetWriter(), oh); }
  osmoh::OpeningHours Deserialize() { return m_serializer.Deserialize(GetReader()); }
  void SerializeEverything() { m_serializer.SetSerializeEverything(); }

  void Flush() { m_bitWriter.reset(); }

  OpeningHoursSerDes m_serializer;

  Buffer m_buffer;
  MemWriter<Buffer> m_memWriter;
  std::unique_ptr<BitWriter<MemWriter<Buffer>>> m_bitWriter;

  std::unique_ptr<MemReader> m_memReader;
  std::unique_ptr<ReaderSource<MemReader>> m_readerSource;
  std::unique_ptr<BitReader<ReaderSource<MemReader>>> m_bitReader;
};

void TestYear(osmoh::RuleSequence const & rule, uint32_t start, uint32_t end)
{
  TEST_EQUAL(rule.GetYears().size(), 1, ());
  TEST_EQUAL(rule.GetYears().back().GetStart(), start, ());
  TEST_EQUAL(rule.GetYears().back().GetEnd(), end, ());
}

void TestWeekday(osmoh::RuleSequence const & rule, Weekday start, Weekday end)
{
  TEST_EQUAL(rule.GetWeekdays().GetWeekdayRanges().size(), 1, ());
  auto const & weekday = rule.GetWeekdays().GetWeekdayRanges()[0];

  TEST_EQUAL(weekday.GetStart(), start, ());
  TEST_EQUAL(weekday.GetEnd(), end, ());
}

void TestHoliday(osmoh::RuleSequence const & rule, bool forSchool)
{
  TEST_EQUAL(rule.GetWeekdays().GetHolidays().size(), 1, ());
  TEST_EQUAL(rule.GetWeekdays().GetHolidays().back().IsPlural(), !forSchool, ());
}

void TestModifier(osmoh::RuleSequence const & rule, osmoh::RuleSequence::Modifier modifier)
{
  TEST_EQUAL(rule.GetModifier(), modifier, ());
}

void TestTime(osmoh::RuleSequence const & rule, uint32_t startH, uint32_t startM, uint32_t endH, uint32_t endM)
{
  TEST_EQUAL(rule.GetTimes().size(), 1, ());
  auto const & time = rule.GetTimes()[0];

  TEST_EQUAL(time.GetStart().GetHoursCount(), startH, ());
  TEST_EQUAL(time.GetStart().GetMinutesCount(), startM, ());

  TEST_EQUAL(time.GetEnd().GetHoursCount(), endH, ());
  TEST_EQUAL(time.GetEnd().GetMinutesCount(), endM, ());
}

void TestMonth(osmoh::RuleSequence const & rule, uint32_t startYear, Month startMonth,
               osmoh::MonthDay::TDayNum startDay, uint32_t endYear, Month endMonth, osmoh::MonthDay::TDayNum endDay)
{
  TEST_EQUAL(rule.GetMonths().size(), 1, ());
  auto const & range = rule.GetMonths().back();

  TEST_EQUAL(range.GetStart().GetYear(), startYear, ());
  TEST_EQUAL(range.GetStart().GetMonth(), startMonth, ());
  TEST_EQUAL(range.GetStart().GetDayNum(), startDay, ());

  TEST_EQUAL(range.GetEnd().GetYear(), endYear, ());
  TEST_EQUAL(range.GetEnd().GetMonth(), endMonth, ());
  TEST_EQUAL(range.GetEnd().GetDayNum(), endDay, ());
}

void TestMonth(osmoh::RuleSequence const & rule, Month startMonth, osmoh::MonthDay::TDayNum startDay, Month endMonth,
               osmoh::MonthDay::TDayNum endDay)
{
  TestMonth(rule, 0 /* startYear */, startMonth, startDay, 0 /* endYear */, endMonth, endDay);
}

void TestMonth(osmoh::RuleSequence const & rule, Month startMonth, Month endMonth)
{
  TestMonth(rule, 0 /* startYear */, startMonth, 0 /* startDay */, 0 /* endYear*/, endMonth, 0 /* endDay */);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_EnableTests_1)
{
  TEST(!IsEnabled(OpeningHoursSerDes::Header::Bits::Year), ());
  Enable(OpeningHoursSerDes::Header::Bits::Year);
  TEST(IsEnabled(OpeningHoursSerDes::Header::Bits::Year), ());

  Enable(OpeningHoursSerDes::Header::Bits::Month);
  TEST(IsEnabled(OpeningHoursSerDes::Header::Bits::Month), ());

  Enable(OpeningHoursSerDes::Header::Bits::Minutes);
  Enable(OpeningHoursSerDes::Header::Bits::Minutes);
  Enable(OpeningHoursSerDes::Header::Bits::Minutes);
  TEST(IsEnabled(OpeningHoursSerDes::Header::Bits::Minutes), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_EnableTests_2)
{
  Enable(OpeningHoursSerDes::Header::Bits::Year);
  Enable(OpeningHoursSerDes::Header::Bits::WeekDay);
  TEST(Serialize("2019 - 2051"), ());
  TEST(Serialize("Mo-Su"), ());
  // Not enabled
  TEST(!Serialize("Apr - May"), ());
  TEST(!Serialize("2019 Nov 30 - 2090 Mar 31"), ());

  Enable(OpeningHoursSerDes::Header::Bits::Month);
  Enable(OpeningHoursSerDes::Header::Bits::MonthDay);
  TEST(Serialize("Apr - May"), ());
  TEST(Serialize("2019 Nov 30 - 2090 Mar 31"), ());
}

// Test on serialization ranges where start is later than end.
// It is wrong but still possible data.
UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_CannotSerialize)
{
  Enable(OpeningHoursSerDes::Header::Bits::Year);
  Enable(OpeningHoursSerDes::Header::Bits::Month);
  TEST(!Serialize("2020 - 2019"), ());
  TEST(!Serialize("2020 May 20 - 2018 Nov 30"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_YearOnly)
{
  Enable(OpeningHoursSerDes::Header::Bits::Year);

  TEST(Serialize("2019 - 2090"), ());
  Flush();

  osmoh::OpeningHours oh = Deserialize();
  TEST(oh.IsValid(), ());
  TEST_EQUAL(oh.GetRule().size(), 1, ());

  auto const & rule = oh.GetRule().back();
  TestYear(rule, 2019, 2090);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_YearAndMonthOnly)
{
  Enable(OpeningHoursSerDes::Header::Bits::Year);
  Enable(OpeningHoursSerDes::Header::Bits::Month);
  Enable(OpeningHoursSerDes::Header::Bits::MonthDay);

  TEST(Serialize("2019 Apr 10 - 2051 May 19"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsValid(), ());
  TEST_EQUAL(oh.GetRule().size(), 1, ());

  auto const & rule = oh.GetRule().back();
  TestMonth(rule, 2019, Month::Apr, 10, 2051, Month::May, 19);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Weekday_1)
{
  Enable(OpeningHoursSerDes::Header::Bits::WeekDay);

  TEST(Serialize("Mo-Fr"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsValid(), ());
  TEST_EQUAL(oh.GetRule().size(), 1, ());

  auto const & rule = oh.GetRule().back();
  TestWeekday(rule, Weekday::Monday, Weekday::Friday);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Weekday_2)
{
  Enable(OpeningHoursSerDes::Header::Bits::WeekDay);

  TEST(Serialize("Mo-Tu, Fr-Su"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsValid(), ());
  TEST_EQUAL(oh.GetRule().size(), 2, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Monday, Weekday::Tuesday);

  rule = oh.GetRule()[1];
  TestWeekday(rule, Weekday::Friday, Weekday::Sunday);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Weekday_Hours_1)
{
  Enable(OpeningHoursSerDes::Header::Bits::WeekDay);
  Enable(OpeningHoursSerDes::Header::Bits::Hours);
  Enable(OpeningHoursSerDes::Header::Bits::Minutes);

  TEST(Serialize("10:00 - 15:00"), ());
  TEST(Serialize("Mo-Fr 09:00 - 23:00"), ());
  TEST(Serialize("Sa - Su 10:30 - 13:00 ; Mo-Fr 00:30 - 23:45"), ());
  Flush();

  {
    auto const oh = Deserialize();
    TEST(oh.IsValid(), ());
    TEST_EQUAL(oh.GetRule().size(), 1, ());
    auto rule = oh.GetRule().back();

    TestTime(rule, 10, 0, 15, 0);
  }
  {
    auto const oh = Deserialize();
    TEST(oh.IsValid(), ());
    TEST_EQUAL(oh.GetRule().size(), 1, ());
    auto rule = oh.GetRule().back();

    TestWeekday(rule, Weekday::Monday, Weekday::Friday);
    TestTime(rule, 9, 0, 23, 0);
  }
  {
    auto const oh = Deserialize();
    TEST(oh.IsValid(), ());
    TEST_EQUAL(oh.GetRule().size(), 2, ());

    auto rule = oh.GetRule()[0];
    TestWeekday(rule, Weekday::Saturday, Weekday::Sunday);
    TestTime(rule, 10, 30, 13, 0);

    rule = oh.GetRule()[1];
    TestWeekday(rule, Weekday::Monday, Weekday::Friday);
    TestTime(rule, 0, 30, 23, 45);
  }
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Off_SerDes_1_AndUsage)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Tu-Su ; Mo OFF"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsValid(), ());
  TEST_EQUAL(oh.GetRule().size(), 2, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Tuesday, Weekday::Sunday);

  rule = oh.GetRule()[1];
  TestWeekday(rule, Weekday::Monday, Weekday::None);
  TestModifier(rule, osmoh::RuleSequence::Modifier::Closed);

  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Thursday, 17 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Monday, 17 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Off_SerDes_2)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Mo off; Tu-Fr 09:00-13:00,14:00-17:00; Sa 09:00-13:00,14:00-16:00; Su off"), ());
  Flush();

  auto const oh = Deserialize();
  TEST_EQUAL(oh.GetRule().size(), 6, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Monday, Weekday::None);
  TestModifier(rule, osmoh::RuleSequence::Modifier::Closed);

  rule = oh.GetRule()[1];
  TestWeekday(rule, Weekday::Tuesday, Weekday::Friday);
  TestTime(rule, 9, 0, 13, 0);

  rule = oh.GetRule()[2];
  TestWeekday(rule, Weekday::Tuesday, Weekday::Friday);
  TestTime(rule, 14, 0, 17, 0);

  rule = oh.GetRule()[3];
  TestWeekday(rule, Weekday::Saturday, Weekday::None);
  TestTime(rule, 9, 0, 13, 0);

  rule = oh.GetRule()[4];
  TestWeekday(rule, Weekday::Saturday, Weekday::None);
  TestTime(rule, 14, 0, 16, 0);

  rule = oh.GetRule()[5];
  TestWeekday(rule, Weekday::Sunday, Weekday::None);
  TestModifier(rule, osmoh::RuleSequence::Modifier::Closed);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_OffJustOff)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("off"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 1, ());

  auto rule = oh.GetRule()[0];
  TestModifier(rule, osmoh::RuleSequence::Modifier::Closed);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_OffJustClosed)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("closed"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 1, ());

  auto rule = oh.GetRule()[0];
  TestModifier(rule, osmoh::RuleSequence::Modifier::Closed);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Open)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("open"), ());
  Flush();

  auto oh = Deserialize();
  TEST_EQUAL(oh.GetRule().size(), 1, ());

  // 1500000000 - Fri Jul 14 05:40:00 2017
  // 1581330500 - Mon Feb 10 13:28:20 2020
  time_t constexpr kDaySeconds = 24 * 3600;
  for (time_t someMoment = 1500000000; someMoment < 1581330500; someMoment += kDaySeconds)
    TEST(oh.IsOpen(someMoment), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_TimeIsOver00)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Mo 19:00-05:00"), ());
  Flush();

  auto oh = Deserialize();
  TEST_EQUAL(oh.GetRule().size(), 1, ());

  TEST(!oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Monday, 13 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Monday, 19 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Monday, 22 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Tuesday, 02 /* hh */, 30 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Tuesday, 06 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_DefaultOpen)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Mo-Sa"), ());
  Flush();

  auto oh = Deserialize();
  TEST_EQUAL(oh.GetRule().size(), 1, ());

  auto const & rule = oh.GetRule().back();
  TestWeekday(rule, Weekday::Monday, Weekday::Saturday);

  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Monday, 13 /* hh */, 30 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Sunday, 13 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_SkipRuleOldYear)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Mo-Fr 07:00-17:00; 2014 Jul 28-2014 Aug 01 off"), ());
  Flush();

  // Skip a rule with an expired year range.
  auto const oh = Deserialize();
  TEST_EQUAL(oh.GetRule().size(), 1, ());
  auto const & rule = oh.GetRule().back();

  TestWeekday(rule, Weekday::Monday, Weekday::Friday);
  TestTime(rule, 7, 0, 17, 0);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_10_plus)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Fr 10:00+"), ());
  Flush();

  // Routing stores the open end as midnight.
  auto const oh = Deserialize();
  TEST_EQUAL(oh.GetRule().size(), 1, ());
  auto const & rule = oh.GetRule().back();

  TestWeekday(rule, Weekday::Friday, Weekday::None);
  TestTime(rule, 10, 0, 0, 0);

  TEST(!oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Mar, Weekday::Friday, 9 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Mar, Weekday::Friday, 10 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Mar, Weekday::Friday, 15 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Mar, Weekday::Friday, 20 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Mar, Weekday::Friday, 23 /* hh */, 30 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Mar, Weekday::Saturday, 00 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_24_7)
{
  EnableAll();

  TEST(Serialize("24/7"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsTwentyFourHours(), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_ExamplesFromOsmAccessConditional_1)
{
  EnableAll();

  TEST(Serialize("Apr 01-Jun 30"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 1, ());
  auto const & rule = oh.GetRule().back();

  TestMonth(rule, Month::Apr, 1, Month::Jun, 30);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_ExamplesFromOsmAccessConditional_2)
{
  EnableAll();

  TEST(Serialize("22:00-06:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 1, ());
  auto const & rule = oh.GetRule().back();

  TestTime(rule, 22, 0, 6, 0);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_ExamplesFromOsmAccessConditional_3)
{
  EnableAll();

  TEST(Serialize("Oct-Mar 18:00-08:00, Apr-Sep 21:30-07:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 2, ());
  auto rule = oh.GetRule()[0];
  TestMonth(rule, Month::Oct, Month::Mar);
  TestTime(rule, 18, 0, 8, 0);

  rule = oh.GetRule()[1];
  TestMonth(rule, Month::Apr, Month::Sep);
  TestTime(rule, 21, 30, 7, 0);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_ExamplesFromOsmAccessConditional_4)
{
  EnableAll();

  TEST(Serialize("apr-oct"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 1, ());
  auto const & rule = oh.GetRule().back();

  TestMonth(rule, Month::Apr, Month::Oct);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_ExamplesFromOsmAccessConditional_5)
{
  EnableAll();

  TEST(Serialize("Mo-Fr 08:30-09:30,15:00-16:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 2, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Monday, Weekday::Friday);
  TestTime(rule, 8, 30, 9, 30);

  rule = oh.GetRule()[1];
  TestWeekday(rule, Weekday::Monday, Weekday::Friday);
  TestTime(rule, 15, 0, 16, 0);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_ExamplesFromOsmAccessConditional_6)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Su,PH 11:00-18:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 2, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Sunday, Weekday::None);

  rule = oh.GetRule()[1];
  TestHoliday(rule, false /* forSchool */);
  TestTime(rule, 11, 0, 18, 0);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_EnableForRouting_1)
{
  EnableForRouting();

  TEST(Serialize("Su,PH 11:00-18:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 1, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Sunday, Weekday::None);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_EnableForRouting_2)
{
  EnableForRouting();

  TEST(Serialize("Mo,Tu,Th,Fr 08:00-08:30,15:00-15:30; We 08:00-08:30,12:00-12:30; SH off"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 10, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Monday, Weekday::None);
  TestTime(rule, 8, 0, 8, 30);

  rule = oh.GetRule()[1];
  TestWeekday(rule, Weekday::Tuesday, Weekday::None);
  TestTime(rule, 8, 0, 8, 30);

  rule = oh.GetRule()[2];
  TestWeekday(rule, Weekday::Thursday, Weekday::None);
  TestTime(rule, 8, 0, 8, 30);

  rule = oh.GetRule()[3];
  TestWeekday(rule, Weekday::Friday, Weekday::None);
  TestTime(rule, 8, 0, 8, 30);

  rule = oh.GetRule()[4];
  TestWeekday(rule, Weekday::Monday, Weekday::None);
  TestTime(rule, 15, 0, 15, 30);

  rule = oh.GetRule()[5];
  TestWeekday(rule, Weekday::Tuesday, Weekday::None);
  TestTime(rule, 15, 0, 15, 30);

  rule = oh.GetRule()[6];
  TestWeekday(rule, Weekday::Thursday, Weekday::None);
  TestTime(rule, 15, 0, 15, 30);

  rule = oh.GetRule()[7];
  TestWeekday(rule, Weekday::Friday, Weekday::None);
  TestTime(rule, 15, 0, 15, 30);

  rule = oh.GetRule()[8];
  TestWeekday(rule, Weekday::Wednesday, Weekday::None);
  TestTime(rule, 8, 0, 8, 30);

  rule = oh.GetRule()[9];
  TestWeekday(rule, Weekday::Wednesday, Weekday::None);
  TestTime(rule, 12, 0, 12, 30);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_WeekdayAndHolidayOff)
{
  SerializeEverything();
  EnableAll();

  TEST(Serialize("Mo-Fr 10:00-18:00; Sa 10:00-13:00; PH off"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 3, ());

  auto rule = oh.GetRule()[0];
  TestWeekday(rule, Weekday::Monday, Weekday::Friday);
  TestTime(rule, 10, 0, 18, 0);

  rule = oh.GetRule()[1];
  TestWeekday(rule, Weekday::Saturday, Weekday::None);
  TestTime(rule, 10, 0, 13, 0);

  rule = oh.GetRule()[2];
  TestHoliday(rule, false /* forSchool */);
  TestModifier(rule, osmoh::RuleSequence::Modifier::Closed);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_WeekDay_OneDay)
{
  EnableAll();

  TEST(Serialize("We 16:00-20:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST_EQUAL(oh.GetRule().size(), 1, ());
  auto const & rule = oh.GetRule().back();

  TestWeekday(rule, Weekday::Wednesday, Weekday::None);
  TestTime(rule, 16, 0, 20, 0);
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Hours_Usage_1)
{
  EnableAll();

  TEST(Serialize("10:00-20:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Thursday, 17 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Monday, 07 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Weekday_Usage_1)
{
  EnableAll();

  TEST(Serialize("We 16:00-20:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsClosed(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Thursday, 17 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Wednesday, 17 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Weekday_Usage_2)
{
  EnableAll();

  TEST(Serialize("Mo-Fr 16:00-20:00, Sa-Su 09:00-14:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsOpen(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Monday, 17 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByWeekday(2020, Month::Feb, Weekday::Thursday, 15 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByWeekday(2023, Month::Apr, Weekday::Saturday, 13 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByWeekday(2023, Month::Apr, Weekday::Saturday, 8 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Weekday_Usage_3)
{
  EnableAll();

  TEST(Serialize("Mo-Su 19:00-13:00"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsClosed(GetUnixtimeByDate(2020, Month::Feb, 6, 17 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2020, Month::Feb, 7, 03 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_Month_Usage)
{
  EnableAll();

  TEST(Serialize("Apr - May"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsOpen(GetUnixtimeByDate(2023, Month::Apr, 8, 03 /* hh */, 30 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByDate(2020, Month::Feb, 5, 17 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MonthDay_Usage)
{
  EnableAll();

  TEST(Serialize("Feb 10 - May 5"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsClosed(GetUnixtimeByDate(2020, Month::Feb, 5, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2020, Month::Feb, 12, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2020, Month::May, 2, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByDate(2020, Month::May, 15, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2021, Month::Mar, 26, 19 /* hh */, 00 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MonthDayYear_Usage)
{
  EnableAll();

  TEST(Serialize("2019 Feb 10 - 2051 May 5"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsClosed(GetUnixtimeByDate(2019, Month::Feb, 5, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Feb, 12, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2020, Month::Feb, 5, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Mar, 26, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByDate(2052, Month::Mar, 26, 19 /* hh */, 00 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MonthHours_Usage)
{
  EnableAll();

  TEST(Serialize("Apr-Nov 01:20-04:50"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsClosed(GetUnixtimeByDate(2019, Month::Feb, 5, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByDate(2020, Month::May, 5, 19 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsClosed(GetUnixtimeByDate(2020, Month::May, 6, 01 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2020, Month::May, 6, 01 /* hh */, 32 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_InverseMonths_Usage_1)
{
  EnableAll();

  TEST(Serialize("Mar - Nov"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Mar, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::May, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Nov, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2019, Month::Dec, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2019, Month::Jan, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2020, Month::Feb, 20, 20 /* hh */, 00 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_InverseMonths_Usage_2)
{
  EnableAll();

  TEST(Serialize("Nov - Mar"), ());
  Flush();

  auto const oh = Deserialize();

  TEST(!oh.IsOpen(GetUnixtimeByDate(2019, Month::Sep, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2019, Month::Oct, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Nov, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Dec, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Jan, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Feb, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2019, Month::Mar, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2020, Month::Apr, 20, 20 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2020, Month::May, 20, 20 /* hh */, 00 /* mm */)), ());
}

// A source rule with several time windows decomposes into several serialized
// rules. After deserialization they must evaluate as a union: with the default
// ";" separator the evaluator would let the parts override each other and keep
// only the 14:00-16:00 window ("no @ (Mo-Fr 07:00-09:00,16:00-18:00)"-style
// conditionals would lose their morning window).
UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MultipleTimeWindows_Evaluation)
{
  EnableForRouting();

  TEST(Serialize("Mo-Fr 10:00-12:00,14:00-16:00"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsValid(), ());

  // 2026-07-06 is a Monday.
  TEST(oh.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 6, 11 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 6, 13 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 6, 15 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 6, 17 /* hh */, 00 /* mm */)), ());
}

// The wire format stores no source-rule boundaries: an overriding open rule
// that overlaps an earlier one would come back widened by the union
// ("Mo-Fr 10:00-18:00; We 12:00-14:00" keeps Wednesday 11:00 closed in the
// source), so such values are refused. Disjoint and closed-overlay rules
// still serialize.
UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_OverridingOverlap_Rejected)
{
  EnableForRouting();
  TEST(!Serialize("Mo-Fr 10:00-18:00; We 12:00-14:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_TwentyFourSevenWithFollowingRule_Rejected)
{
  EnableForRouting();
  // Decomposition would keep only the 24/7 rule and silently drop the closure.
  TEST(!Serialize("24/7; We 12:00-14:00 off"), ());
}

// The WeekDay field stores only a plain range: serializing "Mo[1]" as every
// Monday (or a weekday-in-month date as the whole month) would widen the
// condition, so such values are refused.
UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_NthAndDateWeekdays_Rejected)
{
  EnableForRouting();
  TEST(!Serialize("Mo[1] 10:00-12:00"), ());
  TEST(!Serialize("Feb Mo[1] +2 days 10:00-12:00"), ());
}

// A rule the serializer drops anyway cannot override anything on the wire, so
// it must not make the overlap guard refuse the whole value: ForRouting() does
// not enable Holiday, so only the Mo-Fr part could ever reach the wire.
UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_HolidayDoesNotBlockWeekdayRule)
{
  EnableForRouting();

  TEST(Serialize("Mo-Fr 08:00-18:00; PH 10:00-14:00"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsValid(), ());
  TEST_EQUAL(oh.GetRule().size(), 1, ());
  TestWeekday(oh.GetRule().back(), Weekday::Monday, Weekday::Friday);
  TestTime(oh.GetRule().back(), 8, 0, 18, 0);
}

// Weekday-in-month dates remain in GetRule(), but the wire has no corresponding
// field. Refuse a mixed value whole to avoid partial serialization.
UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MixedWeekdayInMonthDate_Rejected)
{
  EnableForRouting();
  TEST(!Serialize("Mo-Fr 08:00-18:00; Mar Su[-1] 02:00-04:00"), ());
}

// A weekday-in-month date names one day, but the wire format stores a plain
// year/month/day: serializing it would widen the restriction to a whole month.
UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_WeekdayInMonthDate_Rejected)
{
  EnableForRouting();
  TEST(!Serialize("Mar Su[-1] 02:00-04:00"), ());
  TEST(!Serialize("Mar Su[-1]-Oct Su[-1] 09:00-20:30"), ());
  TEST(!Serialize("Feb Mo[1] +2 days 10:00-12:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_DisjointRules_Kept)
{
  EnableForRouting();

  TEST(Serialize("Mo-Fr 10:00-18:00; Sa-Su 12:00-14:00"), ());
  Flush();

  auto const oh = Deserialize();
  TEST(oh.IsValid(), ());

  // 2026-07-08 is a Wednesday, 2026-07-04 is a Saturday.
  TEST(oh.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 8, 11 /* hh */, 00 /* mm */)), ());
  TEST(!oh.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 4, 11 /* hh */, 00 /* mm */)), ());
  TEST(oh.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 4, 13 /* hh */, 00 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_OpenEndedYear_RefusedGracefully)
{
  EnableForRouting();

  // Open-ended year selectors materialize as end year 9999, which does not fit
  // the 8-bit wire encoding; they must refuse, not crash the generator.
  TEST(osmoh::OpeningHours("2020+").IsValid(), ());
  TEST(!Serialize("2020+"), ());
  TEST(!Serialize("2020 Mar 10+ 10:00-18:00"), ());

  // A year range inside the encodable window still serializes.
  auto const now = std::time(nullptr);
  tm nowTm{};
  localtime_r(&now, &nowTm);
  auto const year = std::to_string(nowTm.tm_year + 1900);
  auto const nextYear = std::to_string(nowTm.tm_year + 1901);
  TEST(Serialize(year + "-" + nextYear + " Mo-Fr 10:00-18:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MixedHolidayWeekdayRule_OverlapRefused)
{
  EnableForRouting();

  // The holiday part never reaches the wire, but the weekday part does; the
  // later We rule overrides it, so the deserialized union would widen
  // Wednesday mornings back open.
  TEST(!Serialize("Mo-Fr,PH 08:00-18:00; We 10:00-12:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MixedHolidayWeekdayRule_AsOverrider_Refused)
{
  EnableForRouting();
  TEST(!Serialize("We 06:00-08:00; Mo-Fr,PH 09:00-17:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_MixedHolidayWeekdayRule_Disjoint_Kept)
{
  EnableForRouting();

  // Disjoint weekdays: the union equals the override, so only the holiday
  // part is dropped, exactly as for a single-rule value.
  TEST(Serialize("Mo-Fr,PH 08:00-18:00; Sa 10:00-12:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_TwentyFourSevenAfterSkippedRule_Rejected)
{
  EnableForRouting();

  // The leading holiday rule never serializes, but the trailing closure is
  // real: DecomposeOh would keep only the 24/7 rule and silently lose it.
  TEST(!Serialize("PH 10:00-14:00; 24/7; We 12:00-14:00 off"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_DisjointSeasonalDayRanges_Kept)
{
  EnableForRouting();

  // Disjoint by day numbers although both ranges touch March: the union
  // equals the override, so the value must serialize.
  std::string const source = "Mar 16-Nov 30 08:00-20:00; Dec 01-Mar 15 09:00-16:00";
  TEST(Serialize(source), ());
  Flush();

  auto const roundTrip = Deserialize();
  TEST(roundTrip.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 8, 12 /* hh */, 0 /* mm */)), ());
  TEST(!roundTrip.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 8, 7 /* hh */, 0 /* mm */)), ());
  TEST(roundTrip.IsOpen(GetUnixtimeByDate(2026, Month::Jan, 8, 12 /* hh */, 0 /* mm */)), ());
  TEST(!roundTrip.IsOpen(GetUnixtimeByDate(2026, Month::Jan, 8, 8 /* hh */, 30 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_OverlappingDayRanges_Refused)
{
  EnableForRouting();
  TEST(!Serialize("Mar 01-20 09:00-18:00; Mar 10-15 10:00-12:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_VariableDatesAndOffsets_Rejected)
{
  EnableForRouting();

  // The MonthDay field stores neither variable dates nor date offsets;
  // serializing would turn "easter" into "Jan 01" and drop "+Sa".
  TEST(!Serialize("easter 09:00-18:00"), ());
  TEST(!Serialize("Mar 15 +Sa 09:00-18:00"), ());
  TEST(Serialize("Mar 15 09:00-18:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_UnencodableSelectors_Rejected)
{
  EnableForRouting();

  // The wire format has no field for these; storing the value would degrade
  // it: "week 27" comes back active every week, "2026-2030/2" active in the
  // skipped years, "sunrise-sunset" open around the clock (event times
  // serialize as their 00:00 placeholder) and "/02:00" as continuous hours.
  TEST(!Serialize("week 27 Mo-Fr 10:00-12:00"), ());
  TEST(!Serialize("2026-2030/2 Mo-Fr 10:00-12:00"), ());
  TEST(!Serialize("Mo-Fr sunrise-sunset"), ());
  TEST(!Serialize("Mo-Fr 10:00-18:00/02:00"), ());

  // A stale year range with a period is skipped, so only the live rule is
  // encoded.
  TEST(Serialize("2010-2020/2 Mo-Fr 08:00-10:00; Mo-Fr 10:00-12:00"), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_PartialMonthDayYear_Rejected)
{
  EnableForRouting();

  auto const now = std::time(nullptr);
  tm nowTm{};
  localtime_r(&now, &nowTm);
  auto const year = std::to_string(nowTm.tm_year + 1900);

  // The wire stores years as a range shared by both MonthDay endpoints. A
  // year on only one endpoint cannot be reconstructed losslessly.
  TEST(!m_serializer.Prepare(year + " Jan 01 10:00-12:00"), ());
  TEST(!m_serializer.Prepare(year + " Apr 10:00-12:00"), ());
  TEST(!m_serializer.Prepare(year + " Apr-May 10:00-12:00"), ());
  TEST(!m_serializer.Prepare("Mo-Fr 08:00-10:00; " + year + " Jan 01 10:00-12:00"), ());

  TEST(Serialize(year + " Jan 01-" + year + " Jan 02 10:00-12:00"), ());
  Flush();
  auto const openingHours = Deserialize();
  auto const currentYear = nowTm.tm_year + 1900;
  TEST(openingHours.IsOpen(GetUnixtimeByDate(currentYear, Month::Jan, 1, 11 /* hh */, 0 /* mm */)), ());
  TEST(!openingHours.IsOpen(GetUnixtimeByDate(currentYear + 1, Month::Jan, 1, 11 /* hh */, 0 /* mm */)), ());
}

UNIT_CLASS_TEST(OHSerDesTestFixture, OpeningHoursSerDes_PreparedEncoding_UnalignedRoundTrip)
{
  EnableForRouting();

  auto const encoding = m_serializer.Prepare("Mo-Fr 10:15-18:45");
  TEST(encoding, ());

  Buffer buffer;
  MemWriter<Buffer> memWriter(buffer);
  uint64_t bitsWritten = 0;
  {
    BitWriter<MemWriter<Buffer>> bitWriter(memWriter);
    bitWriter.Write(0b101, 3);
    encoding->Write(bitWriter);
    bitsWritten = bitWriter.BitsWritten();
  }
  TEST_EQUAL(bitsWritten, encoding->GetBitCount() + 3, ());

  MemReader memReader(buffer.data(), buffer.size());
  ReaderSource<MemReader> readerSource(memReader);
  BitReader<ReaderSource<MemReader>> bitReader(readerSource);
  TEST_EQUAL(bitReader.Read(3), 0b101, ());
  auto const openingHours = m_serializer.Deserialize(bitReader);
  TEST(openingHours.IsValid(), ());
  TEST(openingHours.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 8, 12 /* hh */, 0 /* mm */)), ());
  TEST(!openingHours.IsOpen(GetUnixtimeByDate(2026, Month::Jul, 8, 19 /* hh */, 0 /* mm */)), ());
}
}  // namespace opening_hours_serdes_tests
