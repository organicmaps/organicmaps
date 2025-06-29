package app.organicmaps.widget.placepage.sections;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;

import app.organicmaps.sdk.editor.data.HoursMinutes;
import app.organicmaps.sdk.editor.data.Timespan;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.widget.placepage.sections.PlaceOpeningHoursAdapter.WeekScheduleData;
import java.lang.reflect.Field;
import java.util.Calendar;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class PlaceOpeningHoursAdapterTest
{
  PlaceOpeningHoursAdapter adapter;
  Field mWeekScheduleField;

  @Before
  public void setUp() throws NoSuchFieldException
  {
    adapter = spy(new PlaceOpeningHoursAdapter());
    doNothing().when(adapter).notifyDataSetChanged();

    mWeekScheduleField = PlaceOpeningHoursAdapter.class.getDeclaredField("mWeekSchedule");
    mWeekScheduleField.setAccessible(true);
  }

  @Test
  public void test_build_week_from_sunday()
  {
    List<Integer> weekDays = PlaceOpeningHoursAdapter.buildWeekByFirstDay(Calendar.SUNDAY);
    assertEquals(weekDays, List.of(1, 2, 3, 4, 5, 6, 7));
  }

  @Test
  public void test_build_week_from_monday()
  {
    List<Integer> weekDays = PlaceOpeningHoursAdapter.buildWeekByFirstDay(Calendar.MONDAY);
    assertEquals(weekDays, List.of(2, 3, 4, 5, 6, 7, 1));
  }

  @Test
  public void test_build_week_from_saturday()
  {
    List<Integer> weekDays = PlaceOpeningHoursAdapter.buildWeekByFirstDay(Calendar.SATURDAY);
    assertEquals(weekDays, List.of(7, 1, 2, 3, 4, 5, 6));
  }

  @Test
  public void test_build_week_from_friday()
  {
    List<Integer> weekDays = PlaceOpeningHoursAdapter.buildWeekByFirstDay(Calendar.FRIDAY);
    assertEquals(weekDays, List.of(6, 7, 1, 2, 3, 4, 5));
  }

  @Test
  public void test_build_week_errors()
  {
    assertThrows(IllegalArgumentException.class, () -> PlaceOpeningHoursAdapter.buildWeekByFirstDay(-1));
    assertThrows(IllegalArgumentException.class, () -> PlaceOpeningHoursAdapter.buildWeekByFirstDay(0));
    assertThrows(IllegalArgumentException.class, () -> PlaceOpeningHoursAdapter.buildWeekByFirstDay(8));
  }

  @Test
  public void test_single_closed_day_sunday() throws IllegalAccessException
  {
    // opening_hours = "Mo-Fr 09:00-18:00; Sa 12:00-14:00"
    Timetable[] timetables = new Timetable[2];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {2, 3, 4, 5, 6});

    timetables[1] = new Timetable(new Timespan(new HoursMinutes(12, 0, true), new HoursMinutes(14, 0, true)),
                                  new Timespan[0], false, new int[] {7});

    adapter.setTimetables(timetables, Calendar.SUNDAY);

    /* Expected parsed schedule:
     *  0 - Su, closed
     *  1 - Mo-Fr, open
     *  2 - Sa, open
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(3, schedule.size());
    assertTrue(schedule.get(0).isClosed);
    assertEquals(schedule.get(0).startWeekDay, 1);
    assertEquals(schedule.get(0).endWeekDay, 1);
    assertFalse(schedule.get(1).isClosed);
    assertFalse(schedule.get(2).isClosed);
  }

  @Test
  public void test_single_closed_day_monday() throws IllegalAccessException
  {
    // opening_hours = "Mo-Fr 09:00-18:00; Sa 12:00-14:00"
    Timetable[] timetables = new Timetable[2];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {2, 3, 4, 5, 6});

    timetables[1] = new Timetable(new Timespan(new HoursMinutes(12, 0, true), new HoursMinutes(14, 0, true)),
                                  new Timespan[0], false, new int[] {7});

    adapter.setTimetables(timetables, Calendar.MONDAY);

    /* Expected parsed schedule:
     *  0 - Mo-Fr, open
     *  1 - Sa, open
     *  2 - Su, closed
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(3, schedule.size());
    assertFalse(schedule.get(0).isClosed); // Mo-Fr - open
    assertFalse(schedule.get(1).isClosed); // Sa    - open
    assertTrue(schedule.get(2).isClosed); // Su    - closed
    assertEquals(schedule.get(2).startWeekDay, 1);
    assertEquals(schedule.get(2).endWeekDay, 1);
  }

  @Test
  public void test_multiple_closed_day_monday() throws IllegalAccessException
  {
    // opening_hours = "Mo 09:00-18:00; We 10:00-18:00; Fr 11:00-18:00"
    Timetable[] timetables = new Timetable[3];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {2});

    timetables[1] = new Timetable(new Timespan(new HoursMinutes(10, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {4});

    timetables[2] = new Timetable(new Timespan(new HoursMinutes(11, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {6});

    adapter.setTimetables(timetables, Calendar.MONDAY);

    /* Expected parsed schedule:
     *  0 - Mo, open
     *  1 - Tu, closed
     *  2 - We, open
     *  3 - Th, closed
     *  4 - Fr, open
     *  5 - Sa-Su, closed
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(6, schedule.size());
    assertFalse(schedule.get(0).isClosed); //     Mo - open
    assertTrue(schedule.get(1).isClosed); //     Tu - closed
    assertFalse(schedule.get(2).isClosed); //     We - open
    assertTrue(schedule.get(3).isClosed); //     Th - closed
    assertFalse(schedule.get(4).isClosed); //     Fr - open
    assertTrue(schedule.get(5).isClosed); // Sa, Su - closed
    assertEquals(schedule.get(5).startWeekDay, 7);
    assertEquals(schedule.get(5).endWeekDay, 1);
  }

  @Test
  public void test_multiple_closed_day_sunday() throws IllegalAccessException
  {
    // opening_hours = "Mo 09:00-18:00; We 10:00-18:00; Fr 11:00-18:00"
    Timetable[] timetables = new Timetable[3];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {2});

    timetables[1] = new Timetable(new Timespan(new HoursMinutes(10, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {4});

    timetables[2] = new Timetable(new Timespan(new HoursMinutes(11, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {6});

    adapter.setTimetables(timetables, Calendar.SUNDAY);

    /* Expected parsed schedule:
     *  0 - Su, closed
     *  1 - Mo, open
     *  2 - Tu, closed
     *  3 - We, open
     *  4 - Th, closed
     *  5 - Fr, open
     *  6 - Sa, closed
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(7, schedule.size());
    assertTrue(schedule.get(0).isClosed); // Su - closed
    assertFalse(schedule.get(1).isClosed); // Mo - open
    assertTrue(schedule.get(2).isClosed); // Tu - closed
    assertFalse(schedule.get(3).isClosed); // We - open
    assertTrue(schedule.get(4).isClosed); // Th - closed
    assertFalse(schedule.get(5).isClosed); // Fr - open
    assertTrue(schedule.get(6).isClosed); // Sa - closed
    assertEquals(schedule.get(6).startWeekDay, 7);
    assertEquals(schedule.get(6).endWeekDay, 7);
  }

  @Test
  public void test_multiple_closed_days_2_monday() throws IllegalAccessException
  {
    // opening_hours = "Mo 09:00-18:00; We 10:00-18:00; Fr 11:00-18:00"
    Timetable[] timetables = new Timetable[1];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {2, 4, 6});

    adapter.setTimetables(timetables, Calendar.MONDAY);

    /* Expected parsed schedule:
     *  0 - Mo, open
     *  1 - Tu, closed
     *  2 - We, open
     *  3 - Th, closed
     *  4 - Fr, open
     *  5 - Sa-Su, closed
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(6, schedule.size());
    assertFalse(schedule.get(0).isClosed); //     Mo - open
    assertTrue(schedule.get(1).isClosed); //     Tu - closed
    assertFalse(schedule.get(2).isClosed); //     We - open
    assertTrue(schedule.get(3).isClosed); //     Th - closed
    assertFalse(schedule.get(4).isClosed); //     Fr - open
    assertTrue(schedule.get(5).isClosed); // Sa, Su - closed
    assertEquals(schedule.get(5).startWeekDay, 7);
    assertEquals(schedule.get(5).endWeekDay, 1);
  }

  @Test
  public void test_multiple_closed_days_2_sunday() throws IllegalAccessException
  {
    // opening_hours = "Mo 09:00-18:00; We 10:00-18:00; Fr 11:00-18:00"
    Timetable[] timetables = new Timetable[1];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(18, 0, true)),
                                  new Timespan[0], false, new int[] {2, 4, 6});

    adapter.setTimetables(timetables, Calendar.SUNDAY);

    /* Expected parsed schedule:
     *  0 - Su, closed
     *  1 - Mo, open
     *  2 - Tu, closed
     *  3 - We, open
     *  4 - Th, closed
     *  5 - Fr, open
     *  6 - Sa, closed
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(7, schedule.size());
    assertTrue(schedule.get(0).isClosed); // Su - closed
    assertFalse(schedule.get(1).isClosed); // Mo - open
    assertTrue(schedule.get(2).isClosed); // Tu - closed
    assertFalse(schedule.get(3).isClosed); // We - open
    assertTrue(schedule.get(4).isClosed); // Th - closed
    assertFalse(schedule.get(5).isClosed); // Fr - open
    assertTrue(schedule.get(6).isClosed); // Sa - closed
    assertEquals(schedule.get(6).startWeekDay, 7);
    assertEquals(schedule.get(6).endWeekDay, 7);
  }

  @Test
  public void test_open_weekend_sunday() throws IllegalAccessException
  {
    // opening_hours = "Sa-Su 11:00-24:00"
    Timetable[] timetables = new Timetable[1];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(24, 0, true)),
                                  new Timespan[0], false, new int[] {1, 7});

    adapter.setTimetables(timetables, Calendar.SUNDAY);

    /* Expected parsed schedule:
     *  0 - Su, open
     *  1 - Mo-Fr, closed
     *  2 - Sa, open
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(3, schedule.size());
    assertFalse(schedule.get(0).isClosed); // Su - open
    assertTrue(schedule.get(1).isClosed); // Mo-Fr - closed
    assertFalse(schedule.get(2).isClosed); // Sa - open
    assertEquals(schedule.get(0).startWeekDay, 1);
    assertEquals(schedule.get(0).endWeekDay, 1);
    assertEquals(schedule.get(1).startWeekDay, 2);
    assertEquals(schedule.get(1).endWeekDay, 6);
    assertEquals(schedule.get(2).startWeekDay, 7);
    assertEquals(schedule.get(2).endWeekDay, 7);
  }

  @Test
  public void test_open_weekend_monday() throws IllegalAccessException
  {
    // opening_hours = "Sa-Su 11:00-24:00"
    Timetable[] timetables = new Timetable[1];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(24, 0, true)),
                                  new Timespan[0], false, new int[] {1, 7});

    adapter.setTimetables(timetables, Calendar.MONDAY);

    /* Expected parsed schedule:
     *  0 - Mo-Fr, closed
     *  1 - Sa-Su, open
     * */

    List<WeekScheduleData> schedule = (List<WeekScheduleData>) mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(2, schedule.size());
    assertTrue(schedule.get(0).isClosed); // Mo-Fr - closed
    assertFalse(schedule.get(1).isClosed); // Sa-Su - open
    assertEquals(schedule.get(1).startWeekDay, 7);
    assertEquals(schedule.get(1).endWeekDay, 1);
  }
}
