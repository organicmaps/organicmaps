package app.organicmaps.widget.menu;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.res.Resources;
import android.view.View;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import app.organicmaps.R;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;

public class NavMenuTest
{
  @Test
  public void minuteAndHourFieldsRecoverAfterWholeHourAndSubMinuteEstimates() throws Exception
  {
    NavMenu menu = mock(NavMenu.class, CALLS_REAL_METHODS);
    AppCompatActivity activity = mock(AppCompatActivity.class);
    Resources resources = mock(Resources.class);
    when(activity.getResources()).thenReturn(resources);
    when(resources.getString(R.string.minute)).thenReturn("min");
    when(resources.getString(R.string.hour)).thenReturn("h");
    setField(menu, "mActivity", activity);
    TextView hours = mock(TextView.class);
    TextView hourUnits = mock(TextView.class);
    TextView minutes = mock(TextView.class);
    TextView minuteUnits = mock(TextView.class);
    setField(menu, "mTimeHourValue", hours);
    setField(menu, "mTimeHourUnits", hourUnits);
    setField(menu, "mTimeMinuteValue", minutes);
    setField(menu, "mTimeMinuteUnits", minuteUnits);
    Method update = NavMenu.class.getDeclaredMethod("updateTimeLeft", int.class);
    update.setAccessible(true);

    update.invoke(menu, 3570);
    verify(hours).setText("1");
    verify(hours).setVisibility(View.VISIBLE);
    verify(hourUnits).setVisibility(View.VISIBLE);
    verify(minutes).setVisibility(View.GONE);
    verify(minuteUnits).setVisibility(View.GONE);

    clearInvocations(hours, hourUnits, minutes, minuteUnits);
    update.invoke(menu, 45);
    verify(hours).setVisibility(View.GONE);
    verify(hourUnits).setVisibility(View.GONE);
    verify(minutes).setText("\u2066<1\u2069");
    verify(minutes).setVisibility(View.VISIBLE);
    verify(minuteUnits).setVisibility(View.VISIBLE);

    clearInvocations(hours, hourUnits, minutes, minuteUnits);
    update.invoke(menu, 3630);
    verify(hours).setText("1");
    verify(hours).setVisibility(View.VISIBLE);
    verify(minutes).setText("1");
    verify(minutes).setVisibility(View.VISIBLE);
    verify(minuteUnits).setVisibility(View.VISIBLE);
  }

  private static void setField(NavMenu menu, String name, Object value) throws Exception
  {
    Field field = NavMenu.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(menu, value);
  }
}
