package app.organicmaps.util;

import android.app.ActivityManager;
import android.app.Dialog;
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
import java.util.Locale;
import java.util.Set;

public class BatteryOptimizationHelper
{
  private static final String PREF_NAME = "battery_optimization_prefs";
  private static final String KEY_WARNING_SHOWN = "warning_shown";
  private static final Set<String> GUIDE_MANUFACTURERS =
      Set.of("asus", "blackview", "htc", "huawei", "lenovo", "meizu", "motorola", "nokia", "oneplus", "oppo", "realme",
             "samsung", "sony", "stock_android", "tecno", "ulefone", "unihertz", "vivo", "wiko", "xiaomi");

  public static final String REQUEST_KEY = "battery_warning_request";
  public static final String BUNDLE_KEY_ACTION = "action_key";
  public static final int ACTION_GO_TO_SETTINGS = 1;
  public static final int ACTION_CONTINUE = 2;

  public static boolean shouldShowWarning(@NonNull Context context)
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
  public static Intent getSettingsIntent(@NonNull Context context)
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

  public static void showOptimizationDialog(@NonNull FragmentActivity activity)
  {
    WarningDialogFragment dialog = new WarningDialogFragment();
    dialog.show(activity.getSupportFragmentManager(), "BatteryOptimizationWarning");
  }

  @StringRes
  static int warningMessage(@NonNull String manufacturer, int sdk)
  {
    if (sdk >= Build.VERSION_CODES.P && "samsung".equalsIgnoreCase(manufacturer))
      return sdk >= Build.VERSION_CODES.R ? R.string.background_location_samsung_warning_message
                                          : R.string.background_location_samsung_warning_message_legacy;
    return sdk >= Build.VERSION_CODES.S ? R.string.background_location_warning_message
                                        : R.string.background_location_warning_message_legacy;
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
          .setTitle(R.string.background_location_warning_title)
          .setMessage(warningMessage(Build.MANUFACTURER, Build.VERSION.SDK_INT))
          .setPositiveButton(R.string.settings, (dialog, which) -> sendResult(ACTION_GO_TO_SETTINGS))
          .setNegativeButton(R.string.not_now, (dialog, which) -> sendResult(ACTION_CONTINUE))
          .setNeutralButton(R.string.learn_more, null)
          .create();
    }

    @Override
    public void onStart()
    {
      super.onStart();
      // Reading the guide leaves the recording request and its warning available on return.
      ((AlertDialog) requireDialog())
          .getButton(DialogInterface.BUTTON_NEUTRAL)
          .setOnClickListener(view -> Utils.openUrl(requireContext(), instructionsUrl(Build.MANUFACTURER)));
    }

    @Override
    public void onCancel(@NonNull DialogInterface dialog)
    {
      super.onCancel(dialog);
      sendResult(ACTION_CONTINUE);
    }

    private void sendResult(int action)
    {
      markWarningShown(requireContext());
      Bundle result = new Bundle();
      result.putInt(BUNDLE_KEY_ACTION, action);
      getParentFragmentManager().setFragmentResult(REQUEST_KEY, result);
    }
  }
}
