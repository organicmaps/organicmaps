package app.organicmaps.sdk.editor.data;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class Timetable
{
  public final Timespan workingTimespan;
  public final Timespan[] closedTimespans;
  public final boolean isFullday;
  public final int[] weekdays;

  public Timetable(@NonNull Timespan workingTime, @NonNull Timespan[] closedHours, boolean isFullday,
                   @NonNull int[] weekdays)
  {
    this.workingTimespan = workingTime;
    this.closedTimespans = closedHours;
    this.isFullday = isFullday;
    this.weekdays = weekdays;
  }

  public boolean containsWeekday(@IntRange(from = 1, to = 7) int day)
  {
    for (int workingDay : weekdays)
    {
      if (workingDay == day)
        return true;
    }

    return false;
  }

  public boolean isFullWeek()
  {
    return weekdays.length == 7;
  }

  /**
   * Splits the working span into open shifts around its nested breaks, joined by {@code separator},
   * e.g. a lunch break gives "09:00—13:00" + separator + "16:00—20:00". A day fully covered by breaks
   * yields an empty string (caller shows it as closed). Not for full-day rows.
   */
  @NonNull
  public String formatOpenShifts(@NonNull String separator)
  {
    return formatOpenShifts(separator, null, null);
  }

  /**
   * Same as {@link #formatOpenShifts(String)}, but shows the given {@code noon} and {@code midnight}
   * labels instead of 12:00 and 00:00/24:00 shift bounds. In 12-hour locales both render as
   * "12:00 AM" / "12:00 PM", so the labels are the only way to tell them apart.
   */
  @NonNull
  public String formatOpenShifts(@NonNull String separator, @Nullable String noon, @Nullable String midnight)
  {
    final StringBuilder shifts = new StringBuilder();
    HoursMinutes shiftStart = workingTimespan.start;
    for (final Timespan closed : closedTimespans)
    {
      appendShift(shifts, separator, shiftStart, closed.start, noon, midnight);
      shiftStart = closed.end;
    }
    appendShift(shifts, separator, shiftStart, workingTimespan.end, noon, midnight);
    return shifts.toString();
  }

  private static void appendShift(@NonNull StringBuilder shifts, @NonNull String separator, @NonNull HoursMinutes start,
                                  @NonNull HoursMinutes end, @Nullable String noon, @Nullable String midnight)
  {
    // Drop only truly empty shifts. A start later than end is a valid overnight shift, e.g. 23:00—04:00.
    if (start.hours == end.hours && start.minutes == end.minutes)
      return;
    if (shifts.length() > 0)
      shifts.append(separator);
    shifts.append(format(start, noon, midnight)).append('—').append(format(end, noon, midnight));
  }

  @NonNull
  private static String format(@NonNull HoursMinutes hm, @Nullable String noon, @Nullable String midnight)
  {
    if (hm.minutes == 0)
    {
      if (hm.hours == 12 && noon != null)
        return noon;
      if ((hm.hours == 0 || hm.hours == 24) && midnight != null)
        return midnight;
    }
    return hm.toString();
  }

  @Override
  public String toString()
  {
    StringBuilder stringBuilder = new StringBuilder();
    stringBuilder.append("Working timespan : ").append(workingTimespan).append("\n").append("Closed timespans : ");
    for (Timespan timespan : closedTimespans)
      stringBuilder.append(timespan).append("   ");
    stringBuilder.append("\n");
    stringBuilder.append("Fullday : ").append(isFullday).append("\n").append("Weekdays : ");
    for (int i : weekdays)
      stringBuilder.append(i);
    return stringBuilder.toString();
  }
}
