package app.organicmaps.editor.data;

import static org.junit.Assert.assertEquals;

import app.organicmaps.sdk.editor.data.HoursMinutes;
import app.organicmaps.sdk.editor.data.Timespan;
import app.organicmaps.sdk.editor.data.Timetable;
import org.junit.Test;

public class TimeFormatUtilsTest
{
  private static Timespan span(int fromH, int fromM, int toH, int toM)
  {
    return new Timespan(new HoursMinutes(fromH, fromM, true), new HoursMinutes(toH, toM, true));
  }

  private static Timetable workingDay(Timespan working, Timespan... closed)
  {
    return new Timetable(working, closed, false, new int[] {2});
  }

  // The UI (place page rows) stacks shifts with "\n"; long-press-to-copy joins them inline with ", ".
  // Both go through formatOpenShifts, so a break renders the same way in both, never as a labelled
  // "Non-Business Hours" line.

  @Test
  public void formatOpenShifts_noBreak_isWholeWorkingSpan()
  {
    // "Mo 09:00-18:00" -> a single shift, separator unused.
    final Timetable tt = workingDay(span(9, 0, 18, 0));
    assertEquals("09:00—18:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("09:00—18:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }

  @Test
  public void formatOpenShifts_oneBreak_isTwoShifts()
  {
    // "Mo 09:00-13:00,16:00-20:00" -> working 09:00-20:00 with one lunch break -> two shifts.
    final Timetable tt = workingDay(span(9, 0, 20, 0), span(13, 0, 16, 0));
    assertEquals("09:00—13:00\n16:00—20:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("09:00—13:00, 16:00—20:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }

  @Test
  public void formatOpenShifts_twoBreaks_isThreeShifts()
  {
    // "Mo 08:00-12:00,13:00-15:00,16:00-19:00" -> working 08:00-19:00 with two breaks -> three shifts.
    final Timetable tt = workingDay(span(8, 0, 19, 0), span(12, 0, 13, 0), span(15, 0, 16, 0));
    assertEquals("08:00—12:00\n13:00—15:00\n16:00—19:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("08:00—12:00, 13:00—15:00, 16:00—19:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }
}
