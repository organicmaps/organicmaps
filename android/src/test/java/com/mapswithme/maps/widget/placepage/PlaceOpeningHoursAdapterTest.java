package com.mapswithme.maps.widget.placepage;

import com.mapswithme.maps.editor.data.HoursMinutes;
import com.mapswithme.maps.editor.data.Timespan;
import com.mapswithme.maps.editor.data.Timetable;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import org.mockito.junit.MockitoJUnitRunner;

import java.lang.reflect.Field;
import java.util.Calendar;
import java.util.List;

@RunWith(MockitoJUnitRunner.class)
public class PlaceOpeningHoursAdapterTest {
  @Test
  public void test_single_closed_date() throws NoSuchFieldException, IllegalAccessException
  {
    PlaceOpeningHoursAdapter adapter = spy(new PlaceOpeningHoursAdapter());
    doNothing().when(adapter).notifyDataSetChanged();

    Timetable[] timetables = new Timetable[2];
    timetables[0] = new Timetable(new Timespan(new HoursMinutes(9,0, true),
                                               new HoursMinutes(18,0, true)),
                                  new Timespan[0],
                                  false,
                                  new int[]{2, 3, 4, 5, 6});

    timetables[1] = new Timetable(new Timespan(new HoursMinutes(12,0, true),
                                               new HoursMinutes(14,0, true)),
                                  new Timespan[0],
                                  false,
                                  new int[]{7});

    adapter.setTimetables(timetables, Calendar.SUNDAY);

    Field mWeekScheduleField = PlaceOpeningHoursAdapter.class.getDeclaredField("mWeekSchedule");
    mWeekScheduleField.setAccessible(true);
    List<?> schedule = (List<?>)mWeekScheduleField.get(adapter);
    assertNotNull(schedule);
    assertEquals(3, schedule.size());
  }
}
