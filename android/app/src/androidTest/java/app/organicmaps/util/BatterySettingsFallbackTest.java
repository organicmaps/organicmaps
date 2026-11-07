package app.organicmaps.util;

import static org.junit.Assert.assertEquals;

import android.content.ActivityNotFoundException;
import android.content.ContextWrapper;
import android.content.Intent;
import android.provider.Settings;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class BatterySettingsFallbackTest
{
  @Test
  public void missingDestinationFallsBackToSystemSettings()
  {
    final FailingContext context = new FailingContext(1, new ActivityNotFoundException());
    BatteryOptimizationHelper.openSettings(context);
    assertEquals(Settings.ACTION_SETTINGS, context.lastIntent.getAction());
  }

  @Test
  public void protectedDestinationFallsBackToSystemSettings()
  {
    final FailingContext context = new FailingContext(1, new SecurityException());
    BatteryOptimizationHelper.openSettings(context);
    assertEquals(Settings.ACTION_SETTINGS, context.lastIntent.getAction());
  }

  @Test
  public void unavailableSettingsDoNotInterruptTheAdvisory()
  {
    final FailingContext context = new FailingContext(2, new ActivityNotFoundException());
    BatteryOptimizationHelper.openSettings(context);
    assertEquals(Settings.ACTION_SETTINGS, context.lastIntent.getAction());
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
      lastIntent = intent;
      if (mFailuresRemaining-- > 0)
        throw mFailure;
    }
  }
}
