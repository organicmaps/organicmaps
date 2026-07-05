package app.organicmaps.sdk.editor.data;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;

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
   * Splits this working day into its open shifts around the closed (break) spans, joined by {@code separator}.
   * The day is modelled as one working span with its breaks nested inside as closed spans (the C++ editor
   * derives this from the parsed OpenStreetMap rules), so N ordered closed spans yield up to N+1 open shifts,
   * e.g. a lunch break gives "09:00—13:00" + separator + "16:00—20:00". Zero-length shifts (a break touching
   * the working-span boundary or an adjacent break) are dropped, so a day fully covered by breaks yields an
   * empty string. A shift whose end is earlier than its start crosses midnight and is still open time. Must not
   * be called for full-day rows.
   */
  @NonNull
  public String formatOpenShifts(@NonNull String separator)
  {
    final StringBuilder shifts = new StringBuilder();
    HoursMinutes shiftStart = workingTimespan.start;
    for (final Timespan closed : closedTimespans)
    {
      appendShift(shifts, separator, shiftStart, closed.start);
      shiftStart = closed.end;
    }
    appendShift(shifts, separator, shiftStart, workingTimespan.end);
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
