#include "testing/testing.hpp"

#include "editor/opening_hours_ui.hpp"

#include <set>

using namespace editor::ui;

UNIT_TEST(TestTimeTable)
{
  {
    TimeTable tt = TimeTable::GetUninitializedTimeTable();
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

    TEST_EQUAL(tt.GetOpeningDays(), (std::set<osmoh::Weekday>{osmoh::Weekday::Saturday}), ());
  }
}

UNIT_TEST(TestTimeTable_ExcludeTime)
{
  using osmoh::operator""_h;
  using osmoh::operator""_min;
  using osmoh::HourMinutes;

  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 18_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 11_h}), ());
    TEST(tt.AddExcludeTime({12_h, 13_h}), ());
    TEST(tt.AddExcludeTime({15_h, 17_h}), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 3, ());

    TEST(tt.SetOpeningTime({8_h + 15_min, 12_h + 30_min}), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 1, ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 18_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 12_h}), ());
    TEST(tt.AddExcludeTime({11_h, 13_h}), ());
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
    TEST(tt.ReplaceExcludeTime({10_h + 30_min, 14_h}, 1), ());
    TEST_EQUAL(tt.GetExcludeTime().size(), 2, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetStart().GetHourMinutes().GetHoursCount(), 10, ());
    TEST_EQUAL(tt.GetExcludeTime()[0].GetEnd().GetHourMinutes().GetHoursCount(), 14, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetStart().GetHourMinutes().GetHoursCount(), 15, ());
    TEST_EQUAL(tt.GetExcludeTime()[1].GetEnd().GetHourMinutes().GetHoursCount(), 17, ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h + 15_min, 23_h + 30_min});
    TEST(tt.AddExcludeTime({10_h, 11_h}), ());
    TEST(tt.AddExcludeTime({12_h, 13_h}), ());
    TEST(tt.AddExcludeTime({15_h, 17_h}), ());
    TEST_EQUAL(tt.GetPredefinedExcludeTime().GetStart().GetHourMinutes().GetHoursCount(), 20, ());
    TEST_EQUAL(tt.GetPredefinedExcludeTime().GetStart().GetHourMinutes().GetMinutesCount(), 0, ());
    TEST_EQUAL(tt.GetPredefinedExcludeTime().GetEnd().GetHourMinutes().GetHoursCount(), 21, ());
    TEST_EQUAL(tt.GetPredefinedExcludeTime().GetEnd().GetHourMinutes().GetMinutesCount(), 0, ());

    TEST(tt.AddExcludeTime({18_h, 23_h}), ());
    auto const predefinedStart = tt.GetPredefinedExcludeTime().GetStart().GetHourMinutes();
    auto const predefinedEnd = tt.GetPredefinedExcludeTime().GetEnd().GetHourMinutes();
    TEST(predefinedStart.GetHours() == HourMinutes::THours::zero(), ());
    TEST(predefinedStart.GetMinutes() == HourMinutes::TMinutes::zero(), ());
    TEST(predefinedEnd.GetHours() == HourMinutes::THours::zero(), ());
    TEST(predefinedEnd.GetMinutes() == HourMinutes::TMinutes::zero(), ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({8_h, 7_h});
    auto const predefinedStart = tt.GetPredefinedExcludeTime().GetStart().GetHourMinutes();
    auto const predefinedEnd = tt.GetPredefinedExcludeTime().GetEnd().GetHourMinutes();
    TEST(predefinedStart.GetHours() == HourMinutes::THours::zero(), ());
    TEST(predefinedStart.GetMinutes() == HourMinutes::TMinutes::zero(), ());
    TEST(predefinedEnd.GetHours() == HourMinutes::THours::zero(), ());
    TEST(predefinedEnd.GetMinutes() == HourMinutes::TMinutes::zero(), ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({7_h, 8_h + 45_min});
    TEST(!tt.CanAddExcludeTime(), ());
  }
  {
    auto tt = TimeTable::GetPredefinedTimeTable();
    tt.SetTwentyFourHours(false);

    tt.SetOpeningTime({19_h, 18_h});
    auto const predefinedStart = tt.GetPredefinedExcludeTime().GetStart().GetHourMinutes();
    auto const predefinedEnd = tt.GetPredefinedExcludeTime().GetEnd().GetHourMinutes();
    TEST(predefinedStart.GetHours() == HourMinutes::THours::zero(), ());
    TEST(predefinedStart.GetMinutes() == HourMinutes::TMinutes::zero(), ());
    TEST(predefinedEnd.GetHours() == HourMinutes::THours::zero(), ());
    TEST(predefinedEnd.GetMinutes() == HourMinutes::TMinutes::zero(), ());
  }
}

UNIT_TEST(TestAppendTimeTable)
{
  {
    TimeTableSet tts;
    TEST(!tts.Empty(), ());

    {
      auto tt = tts.Back();

      TEST(tt.RemoveWorkingDay(osmoh::Weekday::Sunday), ());
      TEST(tt.RemoveWorkingDay(osmoh::Weekday::Saturday), ());
      TEST(tt.Commit(), ());

      TEST(tts.Append(tts.GetComplementTimeTable()), ());
      TEST_EQUAL(tts.Back().GetOpeningDays(),
                 (std::set<osmoh::Weekday>{osmoh::Weekday::Sunday, osmoh::Weekday::Saturday}), ());
    }

    {
      auto tt = tts.Front();
      TEST(tt.RemoveWorkingDay(osmoh::Weekday::Monday), ());
      TEST(tt.RemoveWorkingDay(osmoh::Weekday::Tuesday), ());
      TEST(tt.Commit(), ());
    }

    TEST(tts.Append(tts.GetComplementTimeTable()), ());
    TEST_EQUAL(tts.Back().GetOpeningDays(), (std::set<osmoh::Weekday>{osmoh::Weekday::Monday, osmoh::Weekday::Tuesday}),
               ());

    TEST(!tts.GetComplementTimeTable().IsValid(), ());
    TEST(!tts.Append(tts.GetComplementTimeTable()), ());
    TEST_EQUAL(tts.Size(), 3, ());

    TEST(tts.Remove(0), ());
    TEST(tts.Remove(1), ());
    TEST_EQUAL(tts.Size(), 1, ());
    TEST_EQUAL(tts.GetUnhandledDays(),
               (std::set<osmoh::Weekday>{osmoh::Weekday::Monday, osmoh::Weekday::Tuesday, osmoh::Weekday::Wednesday,
                                         osmoh::Weekday::Thursday, osmoh::Weekday::Friday}),
               ());
  }
  {
    TimeTableSet tts;
    auto tt = tts.GetComplementTimeTable();
    tt.AddWorkingDay(osmoh::Weekday::Friday);
    tt.AddWorkingDay(osmoh::Weekday::Saturday);
    tt.AddWorkingDay(osmoh::Weekday::Sunday);

    TEST(tts.Append(tt), ());

    TEST_EQUAL(tts.Size(), 2, ());
    TEST_EQUAL(tts.Front().GetOpeningDays().size(), 4, ());
    TEST_EQUAL(tts.Back().GetOpeningDays().size(), 3, ());

    TEST(!tts.GetComplementTimeTable().IsValid(), ());
  }
  {
    TimeTableSet tts;
    auto tt = tts.GetComplementTimeTable();
    tt.AddWorkingDay(osmoh::Weekday::Friday);

    TEST(tts.Append(tt), ());

    TEST_EQUAL(tts.Size(), 2, ());
    TEST_EQUAL(tts.Front().GetOpeningDays().size(), 6, ());
    TEST_EQUAL(tts.Back().GetOpeningDays().size(), 1, ());

    TEST(!tts.GetComplementTimeTable().IsValid(), ());

    tt = tts.Front();
    tt.AddWorkingDay(osmoh::Weekday::Friday);
    TEST(!tts.Append(tt), ());
    TEST_EQUAL(tts.Front().GetOpeningDays().size(), 6, ());
    TEST_EQUAL(tts.Back().GetOpeningDays().size(), 1, ());
  }
  {
    TimeTableSet tts;

    {
      auto tt = tts.GetComplementTimeTable();
      tt.AddWorkingDay(osmoh::Weekday::Friday);
      TEST(tts.Append(tt), ());
    }

    TEST_EQUAL(tts.Size(), 2, ());
    TEST_EQUAL(tts.Front().GetOpeningDays().size(), 6, ());
    TEST_EQUAL(tts.Back().GetOpeningDays().size(), 1, ());

    auto tt = tts.Front();
    tt.AddWorkingDay(osmoh::Weekday::Friday);
    TEST(!tt.Commit(), ());
  }
}
