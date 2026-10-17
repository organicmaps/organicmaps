package app.organicmaps.util;

import android.app.ActivityManager;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.util.log.Logger;
import java.util.Locale;
import java.util.Set;

public class BatteryOptimizationHelper
{
  private static final String PREF_NAME = "OrganicMapsPrefs";
  private static final String KEY_WARNING_SHOWN = "battery_optimization_warning_seen";
  private static final Set<String> GUIDE_MANUFACTURERS =
      Set.of("asus", "blackview", "htc", "huawei", "lenovo", "meizu", "motorola", "nokia", "oneplus", "oppo", "realme",
             "samsung", "sony", "stock_android", "tecno", "ulefone", "unihertz", "vivo", "wiko", "xiaomi");

  public static final String DISMISS_REQUEST_KEY = "battery_warning_dismissed";

  static boolean shouldShowWarning(@NonNull Context context)
  {
    SharedPreferences prefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
    if (prefs.getBoolean(KEY_WARNING_SHOWN, false))
      return false;
    return isSamsungTarget() || isBackgroundRestricted(context);
  }

  private static boolean isSamsungTarget()
  {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && "samsung".equalsIgnoreCase(Build.MANUFACTURER);
  }

  private static boolean isBackgroundRestricted(@NonNull Context context)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
    {
      ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
      return am != null && am.isBackgroundRestricted();
    }
    return false;
  }

  private static void markWarningShown(@NonNull Context context)
  {
    SharedPreferences prefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
    prefs.edit().putBoolean(KEY_WARNING_SHOWN, true).apply();
  }

  @NonNull
  private static Intent getSettingsIntent(@NonNull Context context)
  {
    // Samsung's Never sleeping apps list: https://developer.samsung.com/mobile/app-management.html
    if (isSamsungTarget())
    {
      Intent intent = new Intent("com.samsung.android.sm.ACTION_OPEN_CHECKABLE_LISTACTIVITY");
      intent.setPackage("com.samsung.android.lool");
      intent.putExtra("activity_type", 2);
      if (intent.resolveActivity(context.getPackageManager()) != null)
        return intent;
    }

    return new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.parse("package:" + context.getPackageName()));
  }

  static void openSettings(@NonNull Context context)
  {
    for (Intent intent : new Intent[] {getSettingsIntent(context), new Intent(Settings.ACTION_SETTINGS)})
    {
      try
      {
        // Settings must outlive the widget activity's temporary task.
        context.startActivity(intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        return;
      }
      catch (ActivityNotFoundException | SecurityException e)
      {
        Logger.w("BatteryOptimizationHelper", "Failed to open battery settings: " + intent, e);
      }
    }
  }

  public static boolean showWarningIfNeeded(@NonNull FragmentActivity activity)
  {
    if (!shouldShowWarning(activity))
      return false;
    WarningDialogFragment dialog = new WarningDialogFragment();
    dialog.show(activity.getSupportFragmentManager(), "BatteryOptimizationWarning");
    markWarningShown(activity);
    return true;
  }

  @StringRes
  static int warningMessage(@NonNull String manufacturer, int sdk)
  {
    if (sdk >= Build.VERSION_CODES.P && "samsung".equalsIgnoreCase(manufacturer))
      return sdk >= Build.VERSION_CODES.R ? R.string.background_recording_samsung_warning_message
                                          : R.string.background_recording_samsung_warning_message_legacy;
    return sdk >= Build.VERSION_CODES.S ? R.string.background_recording_warning_message
                                        : R.string.background_recording_warning_message_legacy;
  }

  @NonNull
  static String instructionsUrl(@NonNull String manufacturer)
  {
    final String normalized = manufacturer.toLowerCase(Locale.ROOT);
    final String vendor = switch (normalized)
    {
      case "hmd global" -> "nokia";
      case "google" -> "stock_android";
      case "redmi", "poco" -> "xiaomi";
      case "rugone" -> "ulefone";
      default -> normalized;
    };
    // Firmware versions have different menus; the vendor guide includes the available variants.
    final String path = GUIDE_MANUFACTURERS.contains(vendor) ? vendor : "";
    return "https://dontkillmyapp.com/" + path + "?0&app=Organic%20Maps";
  }

  public static class WarningDialogFragment extends DialogFragment
  {
    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
    {
      return new AlertDialog.Builder(requireContext())
          .setTitle(R.string.background_recording_warning_title)
          .setMessage(warningMessage(Build.MANUFACTURER, Build.VERSION.SDK_INT))
          .setPositiveButton(R.string.settings,
                             (dialog, which) -> {
                               openSettings(requireContext());
                               notifyDismissed();
                             })
          .setNegativeButton(R.string.not_now, (dialog, which) -> notifyDismissed())
          .setNeutralButton(R.string.learn_more, null)
          .create();
    }

    @Override
    public void onStart()
    {
      super.onStart();
      // Keep the advisory available when returning from the manufacturer's guide.
      ((AlertDialog) requireDialog())
          .getButton(DialogInterface.BUTTON_NEUTRAL)
          .setOnClickListener(view -> Utils.openUrl(requireContext(), instructionsUrl(Build.MANUFACTURER)));
    }

    @Override
    public void onCancel(@NonNull DialogInterface dialog)
    {
      super.onCancel(dialog);
      notifyDismissed();
    }

    private void notifyDismissed()
    {
      // Only user dismissal should finish the widget activity; dialog recreation must keep it open.
      getParentFragmentManager().setFragmentResult(DISMISS_REQUEST_KEY, Bundle.EMPTY);
    }
  }
}
