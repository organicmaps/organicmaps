#include "testing/testing.hpp"

#include "editor/ui2oh.hpp"

#include <sstream>
#include <string>

using namespace osmoh;
using namespace editor;
using namespace editor::ui;

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
    OpeningHours oh("2016 Mo-Fr 08:00-10:00");
    TEST(!oh.IsValid(), ());

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
    OpeningHours oh("Mo-Fr 11:00-17:00; Sa-Su 12:00-16:00; Mo-Fr 11:00-13:00 off");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(MakeTimeTableSet(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());

    auto const tt = tts.Get(0);
    TEST_EQUAL(tts.GetUnhandledDays(), OpeningDays(), ());

    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 11, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 17, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 11, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 13, ());
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

UNIT_TEST(OpeningHours2TimeTableSet_plus)
{
  OpeningHours oh("Mo-Su 11:00+");
  TEST(oh.IsValid(), ());

  TimeTableSet tts;

  TEST(MakeTimeTableSet(oh, tts), ());
  TEST_EQUAL(tts.Size(), 1, ());

  auto const tt = tts.Get(0);
  TEST_EQUAL(tts.GetUnhandledDays(), OpeningDays(), ());

  TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 11, ());
  TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 24, ());
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
