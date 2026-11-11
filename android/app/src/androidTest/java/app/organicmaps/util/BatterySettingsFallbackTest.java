package app.organicmaps.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import android.content.ActivityNotFoundException;
import android.content.ContextWrapper;
import android.content.Intent;
import android.os.Build;
import android.provider.Settings;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class BatterySettingsFallbackTest
{
  private Field mManufacturer;
  private String mOriginalManufacturer;

  @Before
  public void useGenericManufacturer() throws Exception
  {
    mManufacturer = Build.class.getField("MANUFACTURER");
    mManufacturer.setAccessible(true);
    mOriginalManufacturer = Build.MANUFACTURER;
    mManufacturer.set(null, "Google");
  }

  @After
  public void restoreManufacturer() throws Exception
  {
    mManufacturer.set(null, mOriginalManufacturer);
  }

  @Test
  public void availableAppInfoIsOpenedDirectly()
  {
    final FailingContext context = new FailingContext(0, new ActivityNotFoundException());
    BatteryOptimizationHelper.openSettings(context);
    assertEquals(List.of(Settings.ACTION_APPLICATION_DETAILS_SETTINGS), context.actions);
    assertEquals("package:" + context.getPackageName(), context.lastIntent.getDataString());
  }

  @Test
  public void missingDestinationFallsBackToSystemSettings()
  {
    final FailingContext context = new FailingContext(1, new ActivityNotFoundException());
    BatteryOptimizationHelper.openSettings(context);
    assertEquals(List.of(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Settings.ACTION_SETTINGS), context.actions);
  }

  @Test
  public void protectedDestinationFallsBackToSystemSettings()
  {
    final FailingContext context = new FailingContext(1, new SecurityException());
    BatteryOptimizationHelper.openSettings(context);
    assertEquals(List.of(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Settings.ACTION_SETTINGS), context.actions);
  }

  @Test
  public void unavailableSettingsDoNotInterruptTheAdvisory()
  {
    final FailingContext context = new FailingContext(2, new ActivityNotFoundException());
    BatteryOptimizationHelper.openSettings(context);
    assertEquals(List.of(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Settings.ACTION_SETTINGS), context.actions);
  }

  @Test
  @SdkSuppress(minSdkVersion = 28)
  public void unavailableSamsungSettingsFallBackToAppInfo() throws Exception
  {
    mManufacturer.set(null, "samsung");
    for (RuntimeException failure : new RuntimeException[] {new ActivityNotFoundException(), new SecurityException()})
    {
      final FailingContext context = new FailingContext(1, failure);
      BatteryOptimizationHelper.openSettings(context);
      assertEquals(List.of("com.samsung.android.sm.ACTION_OPEN_CHECKABLE_LISTACTIVITY",
                           Settings.ACTION_APPLICATION_DETAILS_SETTINGS),
                   context.actions);
    }
  }

  @Test(expected = IllegalStateException.class)
  public void unexpectedFailureIsNotSuppressed()
  {
    BatteryOptimizationHelper.openSettings(new FailingContext(1, new IllegalStateException()));
  }

  private static class FailingContext extends ContextWrapper
  {
    private int mFailuresRemaining;
    private final RuntimeException mFailure;
    final List<String> actions = new ArrayList<>();
    Intent lastIntent;

    FailingContext(int failures, RuntimeException failure)
    {
      super(InstrumentationRegistry.getInstrumentation().getTargetContext());
      mFailuresRemaining = failures;
      mFailure = failure;
    }

    @Override
    public void startActivity(Intent intent)
    {
      assertNotEquals(0, intent.getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK);
      actions.add(intent.getAction());
      lastIntent = intent;
      if (mFailuresRemaining-- > 0)
        throw mFailure;
    }
  }
}
