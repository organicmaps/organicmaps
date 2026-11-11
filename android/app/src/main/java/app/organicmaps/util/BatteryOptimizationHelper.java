package app.organicmaps.util;

import android.app.ActivityManager;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
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
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Set;

public class BatteryOptimizationHelper
{
  enum Warning
  {
    NONE,
    MANUFACTURER,
    BACKGROUND_RESTRICTION
  }

  private static final String ARG_BACKGROUND_RESTRICTED = "background_restricted";
  // Detected restrictions are session advice; manufacturer reminders retain their acknowledgment.
  private static boolean sRestrictionWarningShown;
  private static final String KEY_WARNING_SHOWN = "battery_optimization_warning_seen";
  private static final Set<String> GUIDE_MANUFACTURERS =
      Set.of("asus", "blackview", "htc", "huawei", "lenovo", "meizu", "motorola", "nokia", "oneplus", "oppo", "realme",
             "samsung", "sony", "stock_android", "tecno", "ulefone", "unihertz", "vivo", "wiko", "xiaomi");

  public static final String DISMISS_REQUEST_KEY = "battery_warning_dismissed";
  public static final String DIALOG_TAG = "BatteryOptimizationWarning";

  @NonNull
  static Warning warningFor(@NonNull Context context)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
      return Warning.NONE;
    if (context.getSystemService(ActivityManager.class).isBackgroundRestricted())
      return sRestrictionWarningShown ? Warning.NONE : Warning.BACKGROUND_RESTRICTION;

    final String guide = manufacturerGuide(Build.MANUFACTURER);
    if (!guide.isEmpty() && !guide.equals("stock_android")
        && !MwmApplication.prefs(context).getBoolean(KEY_WARNING_SHOWN, false))
      return Warning.MANUFACTURER;
    return Warning.NONE;
  }

  static void openSettings(@NonNull Context context, boolean backgroundRestricted)
  {
    final List<Intent> intents = new ArrayList<>();
    // Samsung's Never sleeping apps list: https://developer.samsung.com/mobile/app-management.html
    if (!backgroundRestricted && Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
        && "samsung".equalsIgnoreCase(Build.MANUFACTURER))
      intents.add(new Intent("com.samsung.android.sm.ACTION_OPEN_CHECKABLE_LISTACTIVITY")
                      .setPackage("com.samsung.android.lool")
                      .putExtra("activity_type", 2));
    intents.add(new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                           Uri.fromParts("package", context.getPackageName(), null)));
    intents.add(new Intent(Settings.ACTION_SETTINGS));
    for (Intent intent : intents)
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
    if (activity.getSupportFragmentManager().findFragmentByTag(DIALOG_TAG) != null)
      return true;
    final Warning warning = warningFor(activity);
    if (warning == Warning.NONE)
      return false;
    final boolean restricted = warning == Warning.BACKGROUND_RESTRICTION;
    final Bundle args = new Bundle();
    args.putBoolean(ARG_BACKGROUND_RESTRICTED, restricted);
    final WarningDialogFragment dialog = new WarningDialogFragment();
    dialog.setArguments(args);
    dialog.show(activity.getSupportFragmentManager(), DIALOG_TAG);
    if (restricted)
      sRestrictionWarningShown = true;
    else
      MwmApplication.prefs(activity).edit().putBoolean(KEY_WARNING_SHOWN, true).apply();
    return true;
  }

  @StringRes
  static int warningMessage(@NonNull String manufacturer, int sdk, boolean backgroundRestricted)
  {
    if (!backgroundRestricted && sdk >= Build.VERSION_CODES.P && "samsung".equalsIgnoreCase(manufacturer))
      return sdk >= Build.VERSION_CODES.R ? R.string.background_recording_samsung_warning_message
                                          : R.string.background_recording_samsung_warning_message_legacy;
    return sdk >= Build.VERSION_CODES.S ? R.string.background_recording_warning_message
                                        : R.string.background_recording_warning_message_legacy;
  }

  @NonNull
  private static String manufacturerGuide(@NonNull String manufacturer)
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
    return GUIDE_MANUFACTURERS.contains(vendor) ? vendor : "";
  }

  @NonNull
  static String instructionsUrl(@NonNull String manufacturer)
  {
    // Firmware versions have different menus; the vendor guide includes the available variants.
    // "0" hides score icons; "app" inserts the app name into the instructions.
    return "https://dontkillmyapp.com/" + manufacturerGuide(manufacturer) + "?0&app=Organic%20Maps";
  }

  public static class WarningDialogFragment extends DialogFragment
  {
    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
    {
      return new AlertDialog.Builder(requireContext())
          .setTitle(R.string.background_recording_warning_title)
          .setMessage(warningMessage(Build.MANUFACTURER, Build.VERSION.SDK_INT, isRestrictionWarning()))
          .setPositiveButton(R.string.settings,
                             (dialog, which) -> {
                               openSettings(requireContext(), isRestrictionWarning());
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
      // A restored restriction advisory counts as shown in this app session too.
      if (isRestrictionWarning())
        sRestrictionWarningShown = true;
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

    private boolean isRestrictionWarning()
    {
      return requireArguments().getBoolean(ARG_BACKGROUND_RESTRICTED);
    }

    private void notifyDismissed()
    {
      // Only user dismissal should finish the widget activity; dialog recreation must keep it open.
      getParentFragmentManager().setFragmentResult(DISMISS_REQUEST_KEY, Bundle.EMPTY);
    }
  }
}
