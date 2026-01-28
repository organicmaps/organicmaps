package app.organicmaps.editor.data;

import android.content.res.Resources;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import app.organicmaps.R;
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
   * OpenStreetMap stores a single working span with the closed (break) spans nested inside it, so N ordered
   * closed spans yield N+1 open shifts; with no closed spans this is just the whole working span. E.g. a lunch
   * break renders as "09:00—13:00" + separator + "16:00—20:00". Must not be called for full-day rows.
   */
  @NonNull
  public static String formatOpenShifts(@NonNull Timetable tt, @NonNull String separator)
  {
    final StringBuilder shifts = new StringBuilder();
    var shiftStart = tt.workingTimespan.start;
    for (final Timespan closed : tt.closedTimespans)
    {
      shifts.append(shiftStart).append('—').append(closed.start).append(separator);
      shiftStart = closed.end;
    }
    return shifts.append(shiftStart).append('—').append(tt.workingTimespan.end).toString();
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
      return resources.getString(R.string.daily) + " " + formatOpenShifts(tt, ", ");
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
      final String openTime = tt.isFullday ? Utils.unCapitalize(resources.getString(R.string.editor_time_allday))
                                           : formatOpenShifts(tt, ", ");
      weekSchedule.append(weekdays).append(' ').append(openTime);

      firstRow = false;
    }

    return weekSchedule.toString();
  }
}
