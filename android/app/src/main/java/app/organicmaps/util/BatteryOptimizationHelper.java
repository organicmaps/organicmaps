package app.organicmaps.util;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.PowerManager;
import android.provider.Settings;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import app.organicmaps.R;

public final class BatteryOptimizationHelper
{
  private static final String PREF_FILE = "BatteryOptimizationPrefs";
  private static final String PREF_KEY_WARNING_SHOWN = "warning_shown";

  private BatteryOptimizationHelper() {}

  /**
   * Checks if the app is NOT in the system battery optimization allowlist.
   * Note: This represents allowlist status, not a guaranteed GPS termination risk.
   */
  public static boolean isNotAllowlisted(@NonNull Context context)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
    {
      PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
      if (pm != null)
      {
        return !pm.isIgnoringBatteryOptimizations(context.getPackageName());
      }
    }
    return false;
  }

  public static boolean shouldShowWarning(@NonNull Context context)
  {
    SharedPreferences prefs = context.getSharedPreferences(PREF_FILE, Context.MODE_PRIVATE);
    boolean alreadyShown = prefs.getBoolean(PREF_KEY_WARNING_SHOWN, false);

    // Gate reminder logic: On Samsung API 28+ or non-allowlisted devices, show once
    boolean isSamsungTarget =
        Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && "samsung".equalsIgnoreCase(Build.MANUFACTURER);

    return !alreadyShown && (isSamsungTarget || isNotAllowlisted(context));
  }

  public static void markWarningShown(@NonNull Context context)
  {
    SharedPreferences prefs = context.getSharedPreferences(PREF_FILE, Context.MODE_PRIVATE);
    prefs.edit().putBoolean(PREF_KEY_WARNING_SHOWN, true).apply();
  }

  public static void showOptimizationDialog(@NonNull Context context, @NonNull Runnable onDismissAction)
  {
    boolean isSamsung = "samsung".equalsIgnoreCase(Build.MANUFACTURER);

    AlertDialog.Builder builder =
        new AlertDialog.Builder(context, R.style.MwmTheme_AlertDialog).setTitle(R.string.background_location_title);

    // Provide tailored messaging depending on whether it's Samsung or stock Android
    if (isSamsung)
    {
      builder.setMessage(R.string.background_location_samsung_warning_message);
    }
    else
    {
      builder.setMessage(R.string.background_location_warning_message);
    }

    builder
        .setPositiveButton(R.string.settings,
                           (dialog, which) -> {
                             markWarningShown(context);
                             try
                             {
                               Intent intent;
                               if (isSamsung)
                               {
                                 intent = new Intent(Settings.ACTION_BATTERY_SAVER_SETTINGS);
                               }
                               else
                               {
                                 intent = new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS);
                               }
                               context.startActivity(intent);
                             }
                             catch (ActivityNotFoundException e)
                             {
                               Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                               intent.setData(Uri.parse("package:" + context.getPackageName()));
                               context.startActivity(intent);
                             }
                             onDismissAction.run();
                           })
        .setNegativeButton(R.string.not_now,
                           (dialog, which) -> {
                             markWarningShown(context);
                             onDismissAction.run();
                           })
        .setOnCancelListener(dialog -> {
          markWarningShown(context);
          onDismissAction.run();
        })
        .show();
  }
}
