#include "testing/testing.hpp"

#include "editor/opening_hours_ui.hpp"

using namespace editor::ui;

UNIT_TEST(TestTimeTable)
{
  {
    TimeTable tt;
    TEST(!tt.IsValid(), ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    TEST(tt.IsValid(), ());
    TEST(tt.IsTwentyFourHours(), ());

    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Sunday), ());
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Monday), ());
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Tuesday), ());
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Wednesday), ());
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Thursday), ());
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Friday), ());
    TEST(!tt.RemoveWorkingDay(osmoh::Weekday::Saturday), ());

    TEST_EQUAL(tt.GetWorkingDays(), (set<osmoh::Weekday>{osmoh::Weekday::Saturday}), ());
  }
}

UNIT_TEST(TestTimeTable_ExcludeTime)
{
  using osmoh::operator""_h;
  using osmoh::operator""_min;

  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 18_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 11_h}), ());
    TEST(tt.AddExcludeTime({12_h, 13_h}), ());
    TEST(tt.AddExcludeTime({15_h, 17_h}), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 3, ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 18_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 12_h}), ());
    TEST(!tt.AddExcludeTime({11_h, 13_h}), ());
    TEST(tt.AddExcludeTime({15_h, 17_h}), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 2, ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 18_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 17_h}), ());
    TEST(tt.AddExcludeTime({11_h, 13_h}), ());
    TEST(tt.AddExcludeTime({15_h, 17_h}), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 1, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 10, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 17, ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({11_h + 15_min, 18_h + 30_min});
    TEST(!tt.AddExcludeTime({10_h, 12_h}), ());
    TEST(!tt.AddExcludeTime({11_h, 13_h}), ());
    TEST(tt.AddExcludeTime({15_h, 17_h}), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 1, ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 2_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 15_h}), ());
    TEST(tt.AddExcludeTime({16_h, 2_h}), ());
    TEST(tt.AddExcludeTime({16_h, 22_h}), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 2, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 10, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 15, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetStart().GetHourMinutes().GetHoursCount(), 16, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetEnd().GetHourMinutes().GetHoursCount(), 2, ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 15_h + 30_min});
    TEST(!tt.AddExcludeTime({10_h, 16_h}), ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 15_h + 30_min});
    TEST(!tt.AddExcludeTime({7_h, 14_h}), ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 18_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 11_h}), ());
    TEST(tt.AddExcludeTime({12_h, 13_h}), ());
    TEST(tt.AddExcludeTime({15_h, 17_h}), ());
    TEST(tt.ReplaceExcludeTime({13_h, 14_h}, 1), ());
    TEST(!tt.ReplaceExcludeTime({10_h + 30_min, 14_h}, 1), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 3, ());
  }
}

UNIT_TEST(TestAppendTimeTable)
{
  {
    TimeTableSet tts;
    TEST(!tts.empty(), ());

    auto tt = tts.back();

    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Sunday), ());
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Saturday), ());
    TEST(tts.Replace(tt, 0), ());

    TEST(tts.Append(tts.GetComplementTimeTable()), ());
    TEST_EQUAL(tts.back().GetWorkingDays(), (set<osmoh::Weekday>{osmoh::Weekday::Sunday,
              osmoh::Weekday::Saturday}), ());
    tt = tts.front();
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Monday), ());
    TEST(tt.RemoveWorkingDay(osmoh::Weekday::Tuesday), ());
    TEST(tts.Replace(tt, 0), ());

    TEST(tts.Append(tts.GetComplementTimeTable()), ());
    TEST_EQUAL(tts.back().GetWorkingDays(), (set<osmoh::Weekday>{osmoh::Weekday::Monday,
              osmoh::Weekday::Tuesday}), ());

    TEST(!tts.GetComplementTimeTable().IsValid(), ());
    TEST(!tts.Append(tts.GetComplementTimeTable()), ());
    TEST_EQUAL(tts.size(), 3, ());

    TEST(!tts.Remove(0), ());
    TEST(tts.Remove(1), ());
    TEST_EQUAL(tts.size(), 2, ());
    TEST_EQUAL(tts.GetUnhandledDays(), (set<osmoh::Weekday>{osmoh::Weekday::Sunday,
              osmoh::Weekday::Saturday}), ());
  }
  {
    TimeTableSet tts;
    auto tt = tts.GetComplementTimeTable();
    tt.AddWorkingDay(osmoh::Weekday::Friday);
    tt.AddWorkingDay(osmoh::Weekday::Saturday);
    tt.AddWorkingDay(osmoh::Weekday::Sunday);

    TEST(tts.Append(tt), ());

    TEST_EQUAL(tts.size(), 2, ());
    TEST_EQUAL(tts.front().GetWorkingDays().size(), 4, ());
    TEST_EQUAL(tts.back().GetWorkingDays().size(), 3, ());

    TEST(!tts.GetComplementTimeTable().IsValid(), ());
  }
}
