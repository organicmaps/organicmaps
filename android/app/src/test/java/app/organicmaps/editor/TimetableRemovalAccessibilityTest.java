package app.organicmaps.editor;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.app.Activity;
import android.app.Application;
import android.widget.LinearLayout;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.HoursMinutes;
import app.organicmaps.sdk.editor.data.Timespan;
import app.organicmaps.sdk.editor.data.Timetable;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35}, qualifiers = "en-rUS")
public class TimetableRemovalAccessibilityTest
{
  @Test
  public void reusedRowsDescribeTheCurrentDaysAndAllDayHours() throws Exception
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    activity.setTheme(R.style.MwmTheme);
    SimpleTimetableAdapter adapter = mock(SimpleTimetableAdapter.class, CALLS_REAL_METHODS);
    Timespan hours = new Timespan(new HoursMinutes(9, 0, true), new HoursMinutes(18, 0, true));
    List<Timetable> items = new ArrayList<>(List.of(new Timetable(hours, new Timespan[0], false, new int[] {2, 3, 4}),
                                                    new Timetable(hours, new Timespan[0], true, new int[] {7})));
    Field field = SimpleTimetableAdapter.class.getDeclaredField("mItems");
    field.setAccessible(true);
    field.set(adapter, items);
    var holder = spy(adapter.onCreateViewHolder(new LinearLayout(activity), 0));
    var deletion = holder.itemView.findViewById(R.id.tv__remove_timetable);
    doReturn(0).when(holder).getBindingAdapterPosition();
    holder.onBind();
    String first = deletion.getContentDescription().toString();
    assertTrue(first.contains("09:00-18:00"));
    doReturn(1).when(holder).getBindingAdapterPosition();
    holder.onBind();
    String second = deletion.getContentDescription().toString();
    assertTrue(second.contains(activity.getString(R.string.editor_time_allday)));
    assertFalse(second.contains("09:00"));
    assertNotEquals(first, second);
    assertEquals(android.view.View.VISIBLE, deletion.getVisibility());
  }
}
