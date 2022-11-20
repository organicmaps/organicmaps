package app.organicmaps.util;

import static app.organicmaps.util.Config.KEY_PREF_CRASHLYTICS;
import static app.organicmaps.util.Config.KEY_PREF_STATISTICS;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import androidx.annotation.NonNull;

import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.maplayer.Mode;

import java.io.IOException;
import java.util.Locale;

public final class SharedPropertiesUtils
{
  private static final String PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING = "ShowEmulateBadStorageSetting";
  private static final String PREFS_SHOULD_SHOW_LAYER_MARKER_FOR = "ShouldShowGuidesLayerMarkerFor";
  private static final String PREFS_SHOULD_SHOW_LAYER_TUTORIAL_TOAST = "ShouldShowLayerTutorialToast";
  private static final String PREFS_SHOULD_SHOW_HOW_TO_USE_GUIDES_LAYER_TOAST
      = "ShouldShowHowToUseGuidesLayerToast";

  //Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }

  public static boolean isStatisticsEnabled(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getBoolean(KEY_PREF_STATISTICS, true);
  }

  public static void setStatisticsEnabled(@NonNull Context context, boolean enabled)
  {
    MwmApplication.prefs(context).edit().putBoolean(KEY_PREF_STATISTICS, enabled).apply();
  }

  public static boolean isCrashlyticsEnabled(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getBoolean(KEY_PREF_CRASHLYTICS, true);
  }

  public static void setCrashlyticsEnabled(@NonNull Context context, boolean enabled)
  {
    MwmApplication.prefs(context).edit().putBoolean(KEY_PREF_CRASHLYTICS, enabled).apply();
  }

  public static void setShouldShowEmulateBadStorageSetting(@NonNull Context context, boolean show)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    prefs.edit().putBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, show).apply();
  }

  public static boolean shouldShowEmulateBadStorageSetting(@NonNull Context context)
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    return prefs.getBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, false);
  }

  /**
   * @param context context
   * @throws IOException if "Emulate bad storage" is enabled in preferences
   */
  public static void emulateBadExternalStorage(@NonNull Context context) throws IOException
  {
    SharedPreferences prefs = PreferenceManager
        .getDefaultSharedPreferences(MwmApplication.from(context));
    String key = MwmApplication.from(context).getString(R.string.pref_emulate_bad_external_storage);
    if (prefs.getBoolean(key, false)) {
      throw new IOException("Bad external storage error injection");
    }
  }

  public static boolean shouldShowNewMarkerForLayerMode(@NonNull Context context,
                                                        @NonNull Mode mode)
  {
    switch (mode)
    {
      case SUBWAY:
      case TRAFFIC:
      case ISOLINES:
        return false;
      default:
        return getBoolean(context, PREFS_SHOULD_SHOW_LAYER_MARKER_FOR + mode.name()
                                                                            .toLowerCase(Locale.ENGLISH),
                          true);
    }
  }

  public static boolean shouldShowNewMarkerForLayerMode(@NonNull Context context, @NonNull String mode)
  {
    return shouldShowNewMarkerForLayerMode(context, Mode.valueOf(mode));
  }

  public static boolean shouldShowLayerTutorialToast(@NonNull Context context)
  {
    boolean result = getBoolean(context, PREFS_SHOULD_SHOW_LAYER_TUTORIAL_TOAST, true);
    putBoolean(context, PREFS_SHOULD_SHOW_LAYER_TUTORIAL_TOAST, false);
    return result;
  }

  public static boolean shouldShowHowToUseGuidesLayerToast(@NonNull Context context)
  {
    boolean result = getBoolean(context, PREFS_SHOULD_SHOW_HOW_TO_USE_GUIDES_LAYER_TOAST, true);
    putBoolean(context, PREFS_SHOULD_SHOW_HOW_TO_USE_GUIDES_LAYER_TOAST, false);
    return result;
  }

  public static void setLayerMarkerShownForLayerMode(@NonNull Context context, @NonNull Mode mode)
  {
    putBoolean(context, PREFS_SHOULD_SHOW_LAYER_MARKER_FOR + mode.name()
                                                                 .toLowerCase(Locale.ENGLISH), false);
  }

  public static void setLayerMarkerShownForLayerMode(@NonNull Context context, @NonNull String mode)
  {
    setLayerMarkerShownForLayerMode(context, Mode.valueOf(mode));
  }

  private static boolean getBoolean(@NonNull Context context,  @NonNull String key)
  {
    return getBoolean(context, key, false);
  }

  private static boolean getBoolean(@NonNull Context context,  @NonNull String key, boolean defValue)
  {
    return MwmApplication.prefs(context).getBoolean(key, defValue);
  }

  private static void putBoolean(@NonNull Context context,  @NonNull String key, boolean value)
  {
    MwmApplication.prefs(context)
                  .edit()
                  .putBoolean(key, value)
                  .apply();
  }
}
