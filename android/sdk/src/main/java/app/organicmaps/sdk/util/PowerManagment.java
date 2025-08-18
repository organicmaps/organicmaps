package app.organicmaps.sdk.util;

import static app.organicmaps.sdk.util.Constants.Vendor.HUAWEI;
import static app.organicmaps.sdk.util.Constants.Vendor.XIAOMI;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.PowerManager;
import android.provider.Settings;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.Framework;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class PowerManagment
{
  public static final String POWER_MANAGEMENT_TAG = PowerManagment.class.getName();

  // It should consider to power_managment::Scheme from
  // map/power_management/power_management_schemas.hpp
  public static final int NONE = 0;
  public static final int NORMAL = 1;
  public static final int MEDIUM = 2;
  public static final int HIGH = 3;
  public static final int AUTO = 4;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({NONE, NORMAL, MEDIUM, HIGH, AUTO})
  public @interface SchemeType
  {}

  @SchemeType
  public static int getScheme()
  {
    return Framework.nativeGetPowerManagerScheme();
  }

  public static void setScheme(@SchemeType int value)
  {
    Framework.nativeSetPowerManagerScheme(value);
  }

  public static boolean isSystemPowerSaveMode(@NonNull Context context)
  {
    if (XIAOMI.equalsIgnoreCase(Build.MANUFACTURER))
    {
      final String XIAOMI_SETTING_NAME = "POWER_SAVE_MODE_OPEN";
      final int XIAOMI_SETTING_VALUE = 1;
      return Settings.System.getInt(context.getContentResolver(), XIAOMI_SETTING_NAME, -1) == XIAOMI_SETTING_VALUE;
    }
    else if (HUAWEI.equalsIgnoreCase(Build.MANUFACTURER))
    {
      final String HUAWEI_SETTING_NAME = "SmartModeStatus";
      final int HUAWEI_SETTING_VALUE = 4;
      return Settings.System.getInt(context.getContentResolver(), HUAWEI_SETTING_NAME, -1) == HUAWEI_SETTING_VALUE;
    }

    final PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
    if (pm == null)
      return false;

    // Huawei and Xiaomi always returns false here.
    return pm.isPowerSaveMode();
  }

  public static @Nullable Intent makeSystemPowerSaveSettingIntent(@NonNull Context context)
  {
    if (XIAOMI.equalsIgnoreCase(Build.MANUFACTURER))
    {
      final Intent intent = new Intent();
      intent.setComponent(new ComponentName("com.miui.securitycenter", "com.miui.powercenter.PowerMainActivity"));
      if (Utils.isIntentSupported(context, intent))
        return intent;
    }
    else if (HUAWEI.equalsIgnoreCase(Build.MANUFACTURER))
    {
      final Intent intent = new Intent(Intent.ACTION_POWER_USAGE_SUMMARY);
      if (Utils.isIntentSupported(context, intent))
        return intent;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1)
    {
      final Intent intent = new Intent(Settings.ACTION_BATTERY_SAVER_SETTINGS);
      if (Utils.isIntentSupported(context, intent))
        return intent;
    }
    return null;
  }
}
