package app.organicmaps.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.provider.Settings;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContract;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.core.app.ActivityOptionsCompat;
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
    final FailingLauncher launcher = new FailingLauncher(1, new ActivityNotFoundException());
    assertTrue(BatteryOptimizationHelper.openSettings(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                                                      launcher));
    assertEquals(Settings.ACTION_SETTINGS, launcher.lastIntent.getAction());
  }

  @Test
  public void protectedDestinationFallsBackToSystemSettings()
  {
    final FailingLauncher launcher = new FailingLauncher(1, new SecurityException());
    assertTrue(BatteryOptimizationHelper.openSettings(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                                                      launcher));
    assertEquals(Settings.ACTION_SETTINGS, launcher.lastIntent.getAction());
  }

  @Test
  public void unavailableSettingsReportThatRecordingCanContinue()
  {
    final FailingLauncher launcher = new FailingLauncher(2, new ActivityNotFoundException());
    assertFalse(BatteryOptimizationHelper.openSettings(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                                                       launcher));
  }

  @Test(expected = IllegalStateException.class)
  public void invalidLauncherStateFailsAtTheSource()
  {
    BatteryOptimizationHelper.openSettings(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                                           new FailingLauncher(1, new IllegalStateException()));
  }

  private static class FailingLauncher extends ActivityResultLauncher<Intent>
  {
    private int mFailuresRemaining;
    private final RuntimeException mFailure;
    Intent lastIntent;

    FailingLauncher(int failures, RuntimeException failure)
    {
      mFailuresRemaining = failures;
      mFailure = failure;
    }

    @Override
    public void launch(Intent intent, ActivityOptionsCompat options)
    {
      lastIntent = intent;
      if (mFailuresRemaining-- > 0)
        throw mFailure;
    }

    @Override
    public void unregister()
    {}

    @Override
    public ActivityResultContract<Intent, ?> getContract()
    {
      return new ActivityResultContracts.StartActivityForResult();
    }
  }
}
