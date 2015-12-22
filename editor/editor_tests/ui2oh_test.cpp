#include "testing/testing.hpp"

#include "editor/ui2oh.hpp"

using namespace osmoh;
using namespace editor;
using namespace editor::ui;

UNIT_TEST(ConvertOpeningHours)
{
  {
    OpeningHours oh("Mo-Fr 08:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(ConvertOpeningHours(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetWorkingDays().size(), 5, ());
    TEST_EQUAL(tt.GetOpeningTime().GetStart().GetHourMinutes().GetHoursCount(), 8, ());
    TEST_EQUAL(tt.GetOpeningTime().GetEnd().GetHourMinutes().GetHoursCount(), 22, ());
  }
  {
    OpeningHours oh("Mo-Fr 08:00-12:00, 13:00-22:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(ConvertOpeningHours(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetWorkingDays().size(), 5, ());
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

    TEST(ConvertOpeningHours(oh, tts), ());
    TEST_EQUAL(tts.Size(), 1, ());

    auto const tt = tts.Front();
    TEST(!tt.IsTwentyFourHours(), ());
    TEST_EQUAL(tt.GetWorkingDays().size(), 5, ());
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

    TEST(ConvertOpeningHours(oh, tts), ());
    TEST_EQUAL(tts.Size(), 2, ());

    {
      auto const tt = tts.Get(0);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetWorkingDays().size(), 5, ());
    }

    {
      auto const tt = tts.Get(1);
      TEST(!tt.IsTwentyFourHours(), ());
      TEST_EQUAL(tt.GetWorkingDays().size(), 2, ());
    }
  }
  {
    OpeningHours oh("Jan Mo-Fr 08:00-10:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(!ConvertOpeningHours(oh, tts), ());
  }
  {
    OpeningHours oh("2016 Mo-Fr 08:00-10:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(!ConvertOpeningHours(oh, tts), ());
  }
  {
    OpeningHours oh("week 30 Mo-Fr 08:00-10:00");
    TEST(oh.IsValid(), ());

    TimeTableSet tts;

    TEST(!ConvertOpeningHours(oh, tts), ());
  }
}
