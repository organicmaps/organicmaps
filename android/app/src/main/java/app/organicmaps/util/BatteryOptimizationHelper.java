package app.organicmaps.util;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.PowerManager;
import android.provider.Settings;
import androidx.appcompat.app.AlertDialog;
import androidx.annotation.NonNull;

import app.organicmaps.R;

public class BatteryOptimizationHelper
{
    private static final String PREF_TRACKING_WARNING_SHOWN = "pref_tracking_battery_warning_shown";

    public static boolean isBatteryOptimizationEnabled(@NonNull Context context)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            if (pm != null)
            {
                // Returns true if battery optimization IS enabled (i.e. NOT ignoring optimizations)
                return !pm.isIgnoringBatteryOptimizations(context.getPackageName());
            }
        }
        return false;
    }

    public static boolean isAggressiveOEM()
    {
        String manufacturer = Build.MANUFACTURER.toLowerCase();
        return manufacturer.contains("samsung") ||
                manufacturer.contains("google") ||
                manufacturer.contains("oneplus") ||
                manufacturer.contains("xiaomi") ||
                manufacturer.contains("huawei");
    }

    public static boolean shouldShowWarning(@NonNull Context context)
    {
        // Check if it's Android 9 (API 28) or higher
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
            return false;

        // Check if warning has already been shown/dismissed
        android.content.SharedPreferences prefs = android.preference.PreferenceManager.getDefaultSharedPreferences(context);
        boolean alreadyShown = prefs.getBoolean(PREF_TRACKING_WARNING_SHOWN, false);

        return !alreadyShown && isBatteryOptimizationEnabled(context);
    }

    public static void markWarningShown(@NonNull Context context)
    {
        android.content.SharedPreferences prefs = android.preference.PreferenceManager.getDefaultSharedPreferences(context);
        prefs.edit().putBoolean(PREF_TRACKING_WARNING_SHOWN, true).apply();
    }

    public static void showOptimizationDialog(@NonNull Context context)
    {
        markWarningShown(context);

        new AlertDialog.Builder(context, R.style.MwmTheme_AlertDialog)
                .setTitle(R.string.background_location_title)
                .setMessage(R.string.background_location_warning_message)
                .setPositiveButton(R.string.settings, (dialog, which) -> {
                    try
                    {
                        Intent intent = new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS);
                        context.startActivity(intent);
                    }
                    catch (Exception e)
                    {
                        Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                        intent.setData(Uri.parse("package:" + context.getPackageName()));
                        context.startActivity(intent);
                    }
                })
                .setNegativeButton(R.string.cancel, null)
                .show();
    }
}