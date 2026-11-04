package app.organicmaps.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.SharedPreferences;
import app.organicmaps.R;
import org.junit.Test;

public class BatteryOptimizationHelperTest
{
  @Test
  public void existingBatteryWarningAcknowledgmentIsHonored()
  {
    final Context context = mock(Context.class);
    final SharedPreferences preferences = mock(SharedPreferences.class);
    when(context.getSharedPreferences("OrganicMapsPrefs", Context.MODE_PRIVATE)).thenReturn(preferences);
    when(preferences.getBoolean("battery_optimization_warning_seen", false)).thenReturn(true);

    assertFalse(BatteryOptimizationHelper.shouldShowWarning(context));
  }

  @Test
  public void samsungInstructionsFollowTheSleepListVersion()
  {
    assertEquals(R.string.background_recording_samsung_warning_message_legacy,
                 BatteryOptimizationHelper.warningMessage("Samsung", 28));
    assertEquals(R.string.background_recording_samsung_warning_message_legacy,
                 BatteryOptimizationHelper.warningMessage("samsung", 29));
    assertEquals(R.string.background_recording_samsung_warning_message,
                 BatteryOptimizationHelper.warningMessage("SAMSUNG", 30));
    assertEquals(R.string.background_recording_samsung_warning_message,
                 BatteryOptimizationHelper.warningMessage("samsung", 37));
  }

  @Test
  public void otherDevicesUseTheAvailableBackgroundRestrictionControls()
  {
    assertEquals(R.string.background_recording_warning_message_legacy,
                 BatteryOptimizationHelper.warningMessage("Google", 28));
    assertEquals(R.string.background_recording_warning_message_legacy,
                 BatteryOptimizationHelper.warningMessage("Xiaomi", 30));
    assertEquals(R.string.background_recording_warning_message, BatteryOptimizationHelper.warningMessage("Google", 31));
    assertEquals(R.string.background_recording_warning_message,
                 BatteryOptimizationHelper.warningMessage("unknown", 37));
  }

  @Test
  public void manufacturerGuidesHandleAliasesAndUnknownDevices()
  {
    assertEquals("https://dontkillmyapp.com/samsung?0&app=Organic%20Maps",
                 BatteryOptimizationHelper.instructionsUrl("SaMsUnG"));
    assertEquals("https://dontkillmyapp.com/xiaomi?0&app=Organic%20Maps",
                 BatteryOptimizationHelper.instructionsUrl("POCO"));
    assertEquals("https://dontkillmyapp.com/nokia?0&app=Organic%20Maps",
                 BatteryOptimizationHelper.instructionsUrl("HMD Global"));
    assertEquals("https://dontkillmyapp.com/oneplus?0&app=Organic%20Maps",
                 BatteryOptimizationHelper.instructionsUrl("OnePlus"));
    assertEquals("https://dontkillmyapp.com/stock_android?0&app=Organic%20Maps",
                 BatteryOptimizationHelper.instructionsUrl("Google"));
    assertEquals("https://dontkillmyapp.com/?0&app=Organic%20Maps",
                 BatteryOptimizationHelper.instructionsUrl("unknown"));
  }
}
