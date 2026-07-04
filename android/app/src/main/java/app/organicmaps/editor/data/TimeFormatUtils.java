package app.organicmaps.editor.data;

import android.content.res.Resources;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.HoursMinutes;
import app.organicmaps.sdk.editor.data.Timespan;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.utils.Utils;
import java.text.DateFormatSymbols;
import java.util.Locale;

public class TimeFormatUtils
{
  private TimeFormatUtils() {}

  private static String[] sShortWeekdays;
  private static Locale sCurrentLocale;

  private static void refreshWithCurrentLocale()
  {
    if (!Locale.getDefault().equals(sCurrentLocale))
    {
      sCurrentLocale = Locale.getDefault();
      sShortWeekdays = DateFormatSymbols.getInstance().getShortWeekdays();
      for (int i = 0; i < sShortWeekdays.length; i++)
      {
        sShortWeekdays[i] = Utils.capitalize(sShortWeekdays[i]);
      }
    }
  }

  public static String formatShortWeekday(@IntRange(from = 1, to = 7) int day)
  {
    refreshWithCurrentLocale();
    return sShortWeekdays[day];
  }

  public static String formatWeekdaysRange(int startWeekDay, int endWeekDay)
  {
    refreshWithCurrentLocale();
    if (startWeekDay == endWeekDay)
      return sShortWeekdays[startWeekDay];
    else
      return sShortWeekdays[startWeekDay] + "-" + sShortWeekdays[endWeekDay];
  }

  public static String formatWeekdays(@NonNull Timetable timetable)
  {
    return formatWeekdays(timetable.weekdays);
  }

  public static String formatWeekdays(@NonNull int[] weekdays)
  {
    if (weekdays.length == 0)
      return "";

    refreshWithCurrentLocale();
    final StringBuilder builder = new StringBuilder(sShortWeekdays[weekdays[0]]);
    boolean iteratingRange;
    for (int i = 1; i < weekdays.length;)
    {
      iteratingRange = (weekdays[i] == weekdays[i - 1] + 1);
      if (iteratingRange)
      {
        while (i < weekdays.length && weekdays[i] == weekdays[i - 1] + 1)
          i++;
        builder.append("-").append(sShortWeekdays[weekdays[i - 1]]);
        continue;
      }

      if (i < weekdays.length)
        builder.append(", ").append(sShortWeekdays[weekdays[i]]);

      i++;
    }

    return builder.toString();
  }

  /**
   * Splits a working day into its open shifts around the closed (break) spans, joined by {@code separator}.
   * A {@link Timetable} models a day as one working span with its breaks nested inside as closed spans (the
   * C++ editor derives this from the parsed OpenStreetMap rules), so N ordered closed spans yield up to N+1
   * open shifts, e.g. a lunch break gives "09:00—13:00" + separator + "16:00—20:00". Zero-length shifts (a
   * break touching the working-span boundary or an adjacent break) are dropped, so a working span fully
   * covered by breaks yields an empty string. A shift whose end is earlier than its start crosses midnight and
   * is still open time. Must not be called for full-day rows.
   */
  @NonNull
  public static String formatOpenShifts(@NonNull Timetable tt, @NonNull String separator)
  {
    final StringBuilder shifts = new StringBuilder();
    var shiftStart = tt.workingTimespan.start;
    for (final Timespan closed : tt.closedTimespans)
    {
      appendShift(shifts, separator, shiftStart, closed.start);
      shiftStart = closed.end;
    }
    appendShift(shifts, separator, shiftStart, tt.workingTimespan.end);
    return shifts.toString();
  }

  private static void appendShift(@NonNull StringBuilder shifts, @NonNull String separator, @NonNull HoursMinutes start,
                                  @NonNull HoursMinutes end)
  {
    // Drop only truly empty shifts. A start later than end is a valid overnight shift, e.g. 23:00—04:00.
    if (start.hours == end.hours && start.minutes == end.minutes)
      return;
    if (shifts.length() > 0)
      shifts.append(separator);
    shifts.append(start).append('—').append(end);
  }

  public static String formatTimetables(@NonNull Resources resources, String ohStr, Timetable[] timetables)
  {
    if (timetables == null || timetables.length == 0)
      return ohStr;

    // Generate string "24/7" or "Daily HH:MM—HH:MM". Breaks split the day into open shifts, e.g.
    // "Daily 09:00—13:00, 16:00—20:00".
    if (timetables[0].isFullWeek())
    {
      Timetable tt = timetables[0];
      if (tt.isFullday)
        return resources.getString(R.string.twentyfour_seven);
      final String shifts = formatOpenShifts(tt, ", ");
      final String openTime = shifts.isEmpty() ? Utils.unCapitalize(resources.getString(R.string.day_off)) : shifts;
      return resources.getString(R.string.daily) + " " + openTime;
    }

    // Generate full week multiline string, one line per weekday group. E.g.
    // "Mo-Fr 09:00—13:00, 16:00—20:00
    // Sa 10:00—16:00"
    StringBuilder weekSchedule = new StringBuilder();
    boolean firstRow = true;
    for (Timetable tt : timetables)
    {
      if (!firstRow)
        weekSchedule.append('\n');

      final String weekdays = formatWeekdays(tt);
      String openTime = tt.isFullday ? Utils.unCapitalize(resources.getString(R.string.editor_time_allday))
                                     : formatOpenShifts(tt, ", ");
      // A working day fully covered by breaks has no open shift; show it as closed.
      if (openTime.isEmpty())
        openTime = Utils.unCapitalize(resources.getString(R.string.day_off));
      weekSchedule.append(weekdays).append(' ').append(openTime);

      firstRow = false;
    }

    return weekSchedule.toString();
  }
}
