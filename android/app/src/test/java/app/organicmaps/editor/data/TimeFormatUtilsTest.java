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
  public void formatOpenShifts_overnightNoBreak_isWholeWorkingSpan()
  {
    // "Mo 20:00-04:00" -> a single overnight shift, not a fully closed day.
    final Timetable tt = workingDay(span(20, 0, 4, 0));
    assertEquals("20:00—04:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("20:00—04:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
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

  @Test
  public void formatOpenShifts_overnightWithBreak_keepsPostMidnightShift()
  {
    // Working 20:00-03:00 with a 22:00-23:00 break leaves a valid overnight 23:00—03:00 shift.
    final Timetable tt = workingDay(span(20, 0, 3, 0), span(22, 0, 23, 0));
    assertEquals("20:00—22:00\n23:00—03:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("20:00—22:00, 23:00—03:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }

  @Test
  public void formatOpenShifts_breakAtStart_dropsEmptyLeadingShift()
  {
    // "Mo 11:00-17:00; Mo 11:00-13:00 off" parses to working 11:00-17:00, exclude 11:00-13:00.
    // The 11:00—11:00 shift is empty and must not be emitted.
    final Timetable tt = workingDay(span(11, 0, 17, 0), span(11, 0, 13, 0));
    assertEquals("13:00—17:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("13:00—17:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }

  @Test
  public void formatOpenShifts_breakAtEnd_dropsEmptyTrailingShift()
  {
    // Working 11:00-17:00 with a break 15:00-17:00 up to the closing time -> the 17:00—17:00 shift is dropped.
    final Timetable tt = workingDay(span(11, 0, 17, 0), span(15, 0, 17, 0));
    assertEquals("11:00—15:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("11:00—15:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }

  @Test
  public void formatOpenShifts_adjacentBreaks_dropEmptyMiddleShift()
  {
    // Working 09:00-18:00 with back-to-back breaks 12:00-13:00 and 13:00-15:00 -> the 13:00—13:00 shift is dropped.
    final Timetable tt = workingDay(span(9, 0, 18, 0), span(12, 0, 13, 0), span(13, 0, 15, 0));
    assertEquals("09:00—12:00\n15:00—18:00", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("09:00—12:00, 15:00—18:00", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }

  @Test
  public void formatOpenShifts_fullyCovered_isEmpty()
  {
    // Working 11:00-17:00 fully covered by a 11:00-17:00 break -> no open shift; callers render this as closed.
    final Timetable tt = workingDay(span(11, 0, 17, 0), span(11, 0, 17, 0));
    assertEquals("", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }

  @Test
  public void formatOpenShifts_overnightFullyCovered_isEmpty()
  {
    // Working 20:00-04:00 fully covered by a 20:00-04:00 break -> no open shift.
    final Timetable tt = workingDay(span(20, 0, 4, 0), span(20, 0, 4, 0));
    assertEquals("", TimeFormatUtils.formatOpenShifts(tt, "\n"));
    assertEquals("", TimeFormatUtils.formatOpenShifts(tt, ", "));
  }
}
