package app.organicmaps.util;

import static org.junit.Assert.assertEquals;

import android.app.ActivityManager;
import android.app.Instrumentation;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmApplication;
import java.io.IOException;
import java.lang.reflect.Field;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SdkSuppress(minSdkVersion = 28)
public class BatteryOptimizationPolicyTest
{
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final Context mContext = mInstrumentation.getTargetContext();
  private final SharedPreferences mPreferences = MwmApplication.prefs(mContext);
  private Field mManufacturer;
  private String mOriginalManufacturer;
  private Field mRestrictionWarning;
  private boolean mWasRestrictionWarningShown;
  private boolean mWasAcknowledged;
  private boolean mWasRestricted;

  @Before
  public void captureState() throws Exception
  {
    mManufacturer = Build.class.getField("MANUFACTURER");
    mManufacturer.setAccessible(true);
    mOriginalManufacturer = Build.MANUFACTURER;
    mRestrictionWarning = BatteryOptimizationHelper.class.getDeclaredField("sRestrictionWarningShown");
    mRestrictionWarning.setAccessible(true);
    mWasRestrictionWarningShown = mRestrictionWarning.getBoolean(null);
    mWasAcknowledged = mPreferences.getBoolean("battery_optimization_warning_seen", false);
    mWasRestricted = mContext.getSystemService(ActivityManager.class).isBackgroundRestricted();
    mRestrictionWarning.setBoolean(null, false);
    mPreferences.edit().putBoolean("battery_optimization_warning_seen", false).commit();
    restrictBackground(false);
  }

  @After
  public void restoreState() throws Exception
  {
    mManufacturer.set(null, mOriginalManufacturer);
    mRestrictionWarning.setBoolean(null, mWasRestrictionWarningShown);
    mPreferences.edit().putBoolean("battery_optimization_warning_seen", mWasAcknowledged).commit();
    restrictBackground(mWasRestricted);
  }

  @Test
  public void manufacturerRemindersHonorTheExistingAcknowledgment() throws Exception
  {
    for (String manufacturer : new String[] {"Samsung", "Xiaomi", "POCO", "Huawei", "OnePlus"})
    {
      mManufacturer.set(null, manufacturer);
      mPreferences.edit().putBoolean("battery_optimization_warning_seen", false).commit();
      assertEquals(BatteryOptimizationHelper.Warning.MANUFACTURER, BatteryOptimizationHelper.warningFor(mContext));
      mPreferences.edit().putBoolean("battery_optimization_warning_seen", true).commit();
      assertEquals(BatteryOptimizationHelper.Warning.NONE, BatteryOptimizationHelper.warningFor(mContext));
    }
  }

  @Test
  public void stockAndUnknownDevicesDoNotNeedAManufacturerReminder() throws Exception
  {
    for (String manufacturer : new String[] {"Google", "unknown"})
    {
      mManufacturer.set(null, manufacturer);
      assertEquals(BatteryOptimizationHelper.Warning.NONE, BatteryOptimizationHelper.warningFor(mContext));
    }
  }

  @Test
  public void detectedRestrictionsIgnoreTheManufacturerAcknowledgment() throws Exception
  {
    mManufacturer.set(null, "samsung");
    mPreferences.edit().putBoolean("battery_optimization_warning_seen", true).commit();
    restrictBackground(true);
    assertEquals(BatteryOptimizationHelper.Warning.BACKGROUND_RESTRICTION,
                 BatteryOptimizationHelper.warningFor(mContext));
  }

  private void restrictBackground(boolean restricted) throws IOException
  {
    final String command =
        "cmd appops set " + mContext.getPackageName() + " RUN_ANY_IN_BACKGROUND " + (restricted ? "ignore" : "allow");
    try (ParcelFileDescriptor descriptor = mInstrumentation.getUiAutomation().executeShellCommand(command);
         ParcelFileDescriptor.AutoCloseInputStream input = new ParcelFileDescriptor.AutoCloseInputStream(descriptor))
    {
      while (input.read() != -1)
      {}
    }
    assertEquals(restricted, mContext.getSystemService(ActivityManager.class).isBackgroundRestricted());
  }
}
