package app.organicmaps.widget.placepage.sections;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.editor.data.Timetable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.List;

// Groups per-working-day timetables into the weekly schedule rows shown on the place page.
public class WeekScheduleBuilder
{
  private List<WeekScheduleData> mWeekSchedule = Collections.emptyList();

  public void setTimetables(Timetable[] timetables, int currentDayOfWeek)
  {
    final List<Integer> weekDays = buildWeekByFirstDay(currentDayOfWeek);
    final List<WeekScheduleData> scheduleData = new ArrayList<>();

    // Timetables array contains only working days. We need to fill non-working gaps.
    for (int i = 0; i < weekDays.size(); i++)
    {
      final int weekDay = weekDays.get(i);

      final Timetable tt = findScheduleForWeekDay(timetables, weekDay);
      final int startWeekDay = weekDays.get(i);
      if (tt != null)
      {
        while (i < weekDays.size() && tt.containsWeekday(weekDays.get(i)))
          i++;

        i--;
      }
      else
      {
        // Search next working day in timetables.
        while (i + 1 < weekDays.size())
        {
          if (findScheduleForWeekDay(timetables, weekDays.get(i + 1)) != null)
            break;
          i++;
        }
      }

      final int endWeekDay = weekDays.get(i);
      final boolean isBold = (startWeekDay <= endWeekDay)
                               ? (startWeekDay <= currentDayOfWeek && currentDayOfWeek <= endWeekDay)
                               : (currentDayOfWeek >= startWeekDay || currentDayOfWeek <= endWeekDay);
      scheduleData.add(new WeekScheduleData(startWeekDay, endWeekDay, tt, isBold));
    }

    mWeekSchedule = scheduleData;
  }

  @NonNull
  public List<WeekScheduleData> getWeekSchedule()
  {
    return mWeekSchedule;
  }

  public static List<Integer> buildWeekByFirstDay(int firstDayOfWeek)
  {
    if (firstDayOfWeek < 1 || firstDayOfWeek > 7)
      throw new IllegalArgumentException("First day of week " + firstDayOfWeek + " is out of range [1..7]");

    final List<Integer> list = Arrays.asList(Calendar.SUNDAY, Calendar.MONDAY, Calendar.TUESDAY, Calendar.WEDNESDAY,
                                             Calendar.THURSDAY, Calendar.FRIDAY, Calendar.SATURDAY);
    Collections.rotate(list, 1 - firstDayOfWeek);
    return list;
  }

  public static Timetable findScheduleForWeekDay(Timetable[] tables, int weekDay)
  {
    for (Timetable tt : tables)
      if (tt.containsWeekday(weekDay))
        return tt;

    return null;
  }

  public static class WeekScheduleData
  {
    public final int startWeekDay;
    public final int endWeekDay;
    public final boolean isClosed;
    public final Timetable timetable;
    public final boolean isBold;

    public WeekScheduleData(int startWeekDay, int endWeekDay, Timetable timetable, boolean isBold)
    {
      this.startWeekDay = startWeekDay;
      this.endWeekDay = endWeekDay;
      this.isClosed = timetable == null;
      this.timetable = timetable;
      this.isBold = isBold;
    }
  }
}
