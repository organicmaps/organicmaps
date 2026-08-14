#include "testing/testing.hpp"

#include "editor/ui2oh.hpp"

#include <ctime>
#include <sstream>
#include <string>
#include <string_view>

using namespace osmoh;
using namespace editor;
using namespace editor::ui;

namespace
{
// The simple editor accepts a value only if it can represent it losslessly:
// check that its meaning survives a conversion round trip over a whole week.
void TestLosslessRoundTrip(std::string_view value)
{
  OpeningHours const oh(value);
  TEST(oh.IsValid(), (value));

  TimeTableSet tts;
  TEST(MakeTimeTableSet(oh, tts), (value));

  auto const saved = ToString(MakeOpeningHours(tts));
  OpeningHours const back(saved);
  TEST(back.IsValid(), (value, saved));

  // 2020-01-06 00:00 UTC, a Monday.
  time_t constexpr kWeekStart = 1578268800;
  for (time_t t = kWeekStart; t < kWeekStart + 7 * 24 * 60 * 60; t += 15 * 60)
    TEST_EQUAL(oh.IsOpen(t), back.IsOpen(t), (value, saved, t));
}
}  // namespace

UNIT_TEST(OpeningHours2TimeTableSet)
{
  {
    OpeningHours oh("08:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetOpeningDays().size(), 7, ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 8, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 22, ());
  }
  {
    OpeningHours oh("Mo-Su 11:00-23:00;");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetOpeningDays().size(), 7, ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 11, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 23, ());
  }
  {
    OpeningHours oh(
        "Mo-Su 12:00-15:30, 19:30-23:00;"
        "Fr-Sa 12:00-15:30, 19:30-23:30;");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());
    {
      auto const tt = tts.Front();
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 5, ());
      TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 12, ());
      TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 23, ());

      TEST_EQUAL(tt.GetExcludeTime().size(), 1, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 15, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetMinutesCount(), 30, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 19, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetMinutesCount(), 30, ());
    }
    {
      auto const tt = tts.Back();
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 2, ());
      TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 12, ());
      TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 23, ());
      TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetMinutesCount(), 30, ());

      TEST_EQUAL(tt.GetExcludeTime().size(), 1, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 15, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetMinutesCount(), 30, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 19, ());
      TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetMinutesCount(), 30, ());
    }
  }
  {
    OpeningHours oh("Mo-Fr 08:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetOpeningDays().size(), 5, ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 8, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 22, ());
  }
  {
    OpeningHours oh("Mo-Fr 08:00-12:00, 13:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetOpeningDays().size(), 5, ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 8, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 22, ());

    TEST_EQUAL(tt.GetExcludeTime().size(), 1, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 12, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 13, ());
  }
  {
    OpeningHours oh("Mo-Fr 08:00-10:00, 11:00-12:30, 13:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetOpeningDays().size(), 5, ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 8, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 22, ());

    TEST_EQUAL(tt.GetExcludeTime().size(), 2, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 10, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 11, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetStart().GetHourMinutes().GetHoursCount(), 12, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetEnd().GetHourMinutes().GetHoursCount(), 13, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetStart().GetHourMinutes().GetMinutesCount(), 30, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetEnd().GetHourMinutes().GetMinutesCount(), 0, ());
  }
  {
    OpeningHours oh("Mo-Fr 08:00-10:00; Su, Sa 13:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());

    {
      auto const tt = tts.Get(0);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 5, ());
    }

    {
      auto const tt = tts.Get(1);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 2, ());
    }
  }
  {
    OpeningHours oh("Jan Mo-Fr 08:00-10:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(!MakeTimeTableSet(oh, tts), ());
  }
  {
    // Year selectors parse, but the simple editor cannot represent a year,
    // so it falls back to advanced mode.
    OpeningHours oh("2016 Mo-Fr 08:00-10:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(!MakeTimeTableSet(oh, tts), ());
  }
  {
    OpeningHours oh("week 30 Mo-Fr 08:00-10:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(!MakeTimeTableSet(oh, tts), ());
  }
  {
    OpeningHours oh("Mo-Su 11:00-24:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetOpeningDays().size(), 7, ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 11, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 24, ());
  }
  {
    OpeningHours oh("Mo-Fr 08:00-10:00; Su, Sa 13:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());

    {
      auto const tt = tts.Get(0);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 5, ());
    }

    {
      auto const tt = tts.Get(1);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 2, ());
    }
  }
  {
    OpeningHours oh("Mo-Fr 08:00-13:00,14:00-20:00; Sa 09:00-13:00,14:00-18:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());

    {
      auto const tt = tts.Get(0);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 5, ());
    }

    {
      auto const tt = tts.Get(1);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetOpeningDays().size(), 1, ());
    }
  }
}

UNIT_TEST(OpeningHours2TimeTableSet_off)
{
  {
    OpeningHours oh("Mo-Fr 08:00-13:00,14:00-20:00; Su off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
  }
  {
    OpeningHours oh("Mo-Su 08:00-13:00,14:00-20:00; Sa off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.GetUnhandledDays(), OpeningDays({osmoh::Weekday::Saturday}), ());
  }
  {
    OpeningHours oh("Sa; Su; Sa off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.GetUnhandledDays(),
               OpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday,
                            osmoh::Weekday::Thursday, osmoh::Weekday::Friday, osmoh::Weekday::Saturday}),
               ());
  }
  {
    OpeningHours oh("Mo-Su 08:00-13:00,14:00-20:00; Sa 10:00-11:00 off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());

    auto const tt = tts.Get(1);
    TEST_EQUAL(tt.GetOpeningDays(), OpeningDays({osmoh::Weekday::Saturday}), ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 8, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 20, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 10, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 11, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetStart().GetHourMinutes().GetHoursCount(), 13, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetEnd().GetHourMinutes().GetHoursCount(), 14, ());
  }
  {
    OpeningHours oh("Mo-Su; Sa 10:00-11:00 off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());

    auto const tt = tts.Get(1);

    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 0, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 24, ());

    TEST_EQUAL(tt.GetOpeningDays(), OpeningDays({osmoh::Weekday::Saturday}), ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 10, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 11, ());
  }
  {
    OpeningHours oh("Mo-Fr 11:00-17:00; Sa-Su 12:00-16:00; Tu off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());
    TEST_EQUAL(tts.GetUnhandledDays(), OpeningDays({osmoh::Weekday::Tuesday}), ());
  }
  {
    OpeningHours oh("Mo-Fr 11:00-17:00; Sa-Su 12:00-16:00; Mo-Fr off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    TEST_EQUAL(tts.GetUnhandledDays(),
               OpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday,
                            osmoh::Weekday::Thursday, osmoh::Weekday::Friday}),
               ());
  }
  {
    // The exclusion starts where the opening time does: representing it would
    // require shrinking the opening time, which a time table cannot express.
    OpeningHours oh("Mo-Fr 11:00-17:00; Sa-Su 12:00-16:00; Mo-Fr 11:00-13:00 off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(!MakeTimeTableSet(oh, tts), ());
  }
  {
    OpeningHours oh("Mo off; Tu-Su 09:00-17:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    TEST_EQUAL(tts.GetUnhandledDays(), OpeningDays({osmoh::Weekday::Monday}), ());

    auto const tt = tts.Get(0);

    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 9, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 17, ());
  }
}

UNIT_TEST(TimeTableSt2OpeningHours)
{
  {
    TimeTableSet tts;
    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "24/7", ());
  }
  {
    TimeTableSet tts;
    auto tt = tts.Front();
    TEST(tt.SetOpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday,
                            osmoh::Weekday::Thursday, osmoh::Weekday::Friday, osmoh::Weekday::Saturday,
                            osmoh::Weekday::Sunday}),
         ());

    tt.SetTwentyFourHours(false);
    TEST(tt.SetOpeningTime({8_h, 22_h}), ());
    TEST(tt.Commit(), ());

    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "Mo-Su 08:00-22:00", ());
  }
  {
    TimeTableSet tts;
    auto tt = tts.Front();
    TEST(tt.SetOpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday,
                            osmoh::Weekday::Thursday, osmoh::Weekday::Friday}),
         ());

    tt.SetTwentyFourHours(false);
    TEST(tt.SetOpeningTime({8_h, 22_h}), ());
    TEST(tt.Commit(), ());

    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "Mo-Fr 08:00-22:00", ());
  }
  {
    TimeTableSet tts;

    auto tt = tts.Front();
    TEST(tt.SetOpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday,
                            osmoh::Weekday::Thursday, osmoh::Weekday::Friday}),
         ());

    tt.SetTwentyFourHours(false);
    TEST(tt.SetOpeningTime({8_h, 22_h}), ());
    TEST(tt.AddExcludeTime({12_h, 13_h}), ());
    TEST(tt.Commit(), ());

    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "Mo-Fr 08:00-12:00, 13:00-22:00", ());
  }
  {
    TimeTableSet tts;

    auto tt = tts.Front();
    TEST(tt.SetOpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday,
                            osmoh::Weekday::Thursday, osmoh::Weekday::Friday}),
         ());

    tt.SetTwentyFourHours(false);
    TEST(tt.SetOpeningTime({8_h, 22_h}), ());
    TEST(tt.AddExcludeTime({10_h, 11_h}), ());
    TEST(tt.AddExcludeTime({12_h + 30_min, 13_h}), ());
    TEST(tt.Commit(), ());

    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "Mo-Fr 08:00-10:00, 11:00-12:30, 13:00-22:00", ());
  }
  {
    TimeTableSet tts;

    {
      auto tt = tts.Front();
      TEST(tt.SetOpeningDays({osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday, osmoh::Weekday::Thursday}), ());

      tt.SetTwentyFourHours(false);
      TEST(tt.SetOpeningTime({8_h, 10_h}), ());
      TEST(tt.Commit(), ());
    }
    {
      TimeTable tt = TimeTable::GetUninitializedTimeTable();
      TEST(tt.SetOpeningDays(
               {osmoh::Weekday::Monday, osmoh::Weekday::Friday, osmoh::Weekday::Saturday, osmoh::Weekday::Sunday}),
           ());

      tt.SetTwentyFourHours(false);
      TEST(tt.SetOpeningTime({13_h, 22_h}), ());
      TEST(tts.Append(tt), ());
    }

    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "Tu-Th 08:00-10:00; Fr-Mo 13:00-22:00", ());
  }
  {
    TimeTableSet tts;

    {
      auto tt = tts.Front();
      TEST(tt.SetOpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Wednesday, osmoh::Weekday::Friday}), ());

      tt.SetTwentyFourHours(false);
      TEST(tt.SetOpeningTime({8_h, 10_h}), ());
      TEST(tt.Commit(), ());
    }
    {
      TimeTable tt = TimeTable::GetUninitializedTimeTable();
      TEST(tt.SetOpeningDays({osmoh::Weekday::Saturday, osmoh::Weekday::Sunday}), ());

      tt.SetTwentyFourHours(false);
      TEST(tt.SetOpeningTime({13_h, 22_h}), ());
      TEST(tts.Append(tt), ());
    }

    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "Mo, We, Fr 08:00-10:00; Sa-Su 13:00-22:00", ());
  }
  {
    TimeTableSet tts;

    auto tt = tts.Front();
    TEST(tt.SetOpeningDays({osmoh::Weekday::Sunday, osmoh::Weekday::Monday, osmoh::Weekday::Tuesday,
                            osmoh::Weekday::Wednesday, osmoh::Weekday::Thursday, osmoh::Weekday::Friday,
                            osmoh::Weekday::Saturday}),
         ());

    tt.SetTwentyFourHours(false);
    TEST(tt.SetOpeningTime({11_h, 24_h}), ());
    TEST(tt.Commit(), ());

    TEST_EQUAL(ToString(MakeOpeningHours(tts)), "Mo-Su 11:00-24:00", ());
  }
  {
    TimeTableSet tts;

    {
      auto tt = tts.Front();
      TEST(tt.SetOpeningDays({osmoh::Weekday::Monday, osmoh::Weekday::Wednesday, osmoh::Weekday::Thursday}), ());

      tt.SetTwentyFourHours(false);
      TEST(tt.SetOpeningTime({8_h, 20_h}), ());
      TEST(tt.AddExcludeTime({13_h, 14_h}), ());
      TEST(tt.Commit(), ());
    }
    {
      TimeTable tt = TimeTable::GetUninitializedTimeTable();
      TEST(tt.SetOpeningDays({osmoh::Weekday::Saturday}), ());

      tt.SetTwentyFourHours(false);
      TEST(tt.SetOpeningTime({9_h, 18_h}), ());
      TEST(tt.AddExcludeTime({13_h, 14_h}), ());
      TEST(tts.Append(tt), ());
    }

    TEST_EQUAL(ToString(MakeOpeningHours(tts)),
               "Mo, We-Th 08:00-13:00, 14:00-20:00; "
               "Sa 09:00-13:00, 14:00-18:00",
               ());
  }
}

UNIT_TEST(OpeningHours2TimeTableSet_onlyRepresentableSchedulesUseSimpleMode)
{
  for (std::string_view const value : {
           "Mo-Fr 08:00-18:00; PH off",
           "Mo[1] 08:00-18:00",
           "Mo +1 day 08:00-18:00",
           "Mo-Fr 08:00-18:00 \"office\"",
           "Mo-Fr 08:00-18:00 unknown",
           "24/7 closed",
           "Mo-Fr 08:00-18:00/02:00",
           "Mo-Fr 20:00-26:00",
           "Mo-Su 11:00+",
           "Mo-Su sunrise-sunset",
           "Mo-Fr 10:00-sunset",
           "Mo-Su (sunrise+01:00)-sunset",
           "sunrise-sunset",
           "Mo-Fr 08:00-18:00; Sa sunrise-sunset",
           "Mo-Fr 08:00-18:00 || Sa 10:00-14:00",
       })
  {
    OpeningHours const oh(value);
    TEST(oh.IsValid(), (value));
    TimeTableSet tts;
    TEST(!MakeTimeTableSet(oh, tts), (value));
  }

  for (std::string_view const value : {
           "24/7",
           "open",
           "Mo-Fr 20:00-02:00",
           "Mo-Fr 08:00-18:00; Sa 10:00-14:00",
           "Mo-Fr 08:00-24:00",
       })
  {
    TestLosslessRoundTrip(value);
  }
}

// A closed rule overrides everything it intersects; losing any part of it on
// save would reopen closed hours.
UNIT_TEST(OpeningHours2TimeTableSet_closedOverridesApplyEverywhere)
{
  {
    // A trailing constant "off" closes the whole week: not representable.
    OpeningHours oh("Mo-Fr 08:00-18:00; off");
    TEST(oh.IsValid(), ());
    TimeTableSet tts;
    TEST(!MakeTimeTableSet(oh, tts), ());
  }
  {
    // A whole-week override closes both time tables: nothing left to edit.
    OpeningHours oh("Mo-Fr 08:00-18:00; Sa-Su 10:00-16:00; Mo-Su off");
    TEST(oh.IsValid(), ());
    TimeTableSet tts;
    TEST(!MakeTimeTableSet(oh, tts), ());
  }
  {
    // A selectorless closed rule applies to the whole week.
    TestLosslessRoundTrip("Mo-Fr 08:00-18:00; 13:00-14:00 off");

    OpeningHours oh("Mo-Fr 08:00-18:00; 13:00-14:00 off");
    TEST(oh.IsValid(), ());
    TimeTableSet tts;
    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST_EQUAL(tt.GetExcludeTime().size(), 1, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 13, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 14, ());
  }
  {
    // The override reaches the second time table, not only the first one.
    TestLosslessRoundTrip("Mo-Fr 08:00-18:00; Sa-Su 10:00-16:00; Sa 12:00-13:00 off");

    OpeningHours oh("Mo-Fr 08:00-18:00; Sa-Su 10:00-16:00; Sa 12:00-13:00 off");
    TEST(oh.IsValid(), ());
    TimeTableSet tts;
    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 3, ());

    TEST_EQUAL(tts.Get(0).GetOpeningDays().size(), 5, ());
    // Saturday is split out of Sa-Su and carries the exclusion.
    TEST_EQUAL(tts.Get(1).GetOpeningDays().size(), 1, ());
    TEST_EQUAL(tts.Get(2).GetOpeningDays().size(), 1, ());
    TEST_EQUAL(tts.Get(2).GetExcludeTime().size(), 1, ());
  }
  {
    // The override closes a whole time table, but not the whole week.
    TestLosslessRoundTrip("Mo-Fr 08:00-18:00; Sa-Su 10:00-16:00; Sa-Su 10:00-16:00 off");

    OpeningHours oh("Mo-Fr 08:00-18:00; Sa-Su 10:00-16:00; Sa-Su 10:00-16:00 off");
    TimeTableSet tts;
    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());
    TEST_EQUAL(tts.GetUnhandledDays(), OpeningDays({osmoh::Weekday::Saturday, osmoh::Weekday::Sunday}), ());
  }
  {
    // The override closes a part of a time table completely.
    TestLosslessRoundTrip("Mo-Fr 08:00-18:00; Mo 08:00-18:00 off");

    OpeningHours oh("Mo-Fr 08:00-18:00; Mo 08:00-18:00 off");
    TimeTableSet tts;
    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());
    TEST_EQUAL(tts.Get(0).GetOpeningDays(),
               OpeningDays({osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday, osmoh::Weekday::Thursday,
                            osmoh::Weekday::Friday}),
               ());
  }

  // A closed span that only overlaps the opening one would reopen closed hours:
  // excluding it as is leaves a zero-length span, which means open all day.
  for (std::string_view const value : {
           "Mo-Fr 08:00-18:00; Mo 07:00-12:00 off",
           "Mo-Fr 08:00-18:00; Mo 12:00-20:00 off",
           "Mo-Fr 08:00-18:00; Mo-Fr 08:00-18:00 off",
       })
  {
    OpeningHours const oh(value);
    TEST(oh.IsValid(), (value));
    TimeTableSet tts;
    TEST(!MakeTimeTableSet(oh, tts), (value));
  }
}

// The simple editing model is linear in wall-clock time: a lone overnight
// span fits it, but combinations do not survive a round trip.
UNIT_TEST(OpeningHours2TimeTableSet_overnightCombinationsStayInAdvancedMode)
{
  for (std::string_view const value : {
           "Mo 20:00-02:00, 01:00-04:00",           // wrapped span plus a second span
           "Mo 20:00-02:00; Mo 21:00-23:00 off",    // exclusion inside a wrapped span
           "Mo-Fr 08:00-18:00; Sa 23:00-01:00 off"  // wrapping exclusion span
       })
  {
    OpeningHours const oh(value);
    TEST(oh.IsValid(), (value));
    TimeTableSet tts;
    TEST(!MakeTimeTableSet(oh, tts), (value));
  }

  // A closed rule on disjoint days does not disturb an overnight time table.
  TestLosslessRoundTrip("Mo 20:00-02:00; Tu 12:00-13:00 off");

  OpeningHours const disjoint("Mo 20:00-02:00; Tu 12:00-13:00 off");
  TimeTableSet tts;
  TEST(MakeTimeTableSet(disjoint, tts), ());
  TEST_EQUAL(tts.Size(), 1, ());
  TEST_EQUAL(tts.Front().GetOpeningDays().size(), 1, ());
}

// An additive rule over days disjoint from every preceding rule is equivalent
// to an overriding one, so the simple editor represents it losslessly; place
// pages then render it structured instead of as raw text.
UNIT_TEST(OpeningHours2TimeTableSet_disjointAdditiveRules)
{
  {
    TestLosslessRoundTrip("Mo-Fr 08:00-18:00, Sa 10:00-14:00");

    OpeningHours oh("Mo-Fr 08:00-18:00, Sa 10:00-14:00");
    TEST(oh.IsValid(), ());
    TimeTableSet tts;
    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());
    TEST_EQUAL(tts.Get(0).GetOpeningDays().size(), 5, ());
    TEST_EQUAL(tts.Get(1).GetOpeningDays().size(), 1, ());
  }

  // Overlapping days or a closed additive rule change semantics under the
  // overriding model: keep them in the advanced editor.
  for (std::string_view const value : {"Mo-Fr 08:00-18:00, Mo 10:00-14:00", "Mo-Fr 08:00-18:00, Sa off"})
  {
    OpeningHours const oh(value);
    TEST(oh.IsValid(), (value));
    TimeTableSet tts;
    TEST(!MakeTimeTableSet(oh, tts), (value));
  }
}
