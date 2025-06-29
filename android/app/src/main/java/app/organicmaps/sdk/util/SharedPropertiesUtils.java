package app.organicmaps.sdk.util;

import static app.organicmaps.sdk.util.Config.KEY_PREF_STATISTICS;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.maplayer.Mode;
import java.io.IOException;
import java.util.Locale;

public final class SharedPropertiesUtils
{
  private static final String PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING = "ShowEmulateBadStorageSetting";
  private static final String PREFS_SHOULD_SHOW_LAYER_MARKER_FOR = "ShouldShowGuidesLayerMarkerFor";

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static SharedPreferences mPrefs;

  // Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }

  public static void init(@NonNull SharedPreferences prefs)
  {
    mPrefs = prefs;
  }

  public static boolean isStatisticsEnabled()
  {
    return mPrefs.getBoolean(KEY_PREF_STATISTICS, true);
  }

  public static void setShouldShowEmulateBadStorageSetting(boolean show)
  {
    mPrefs.edit().putBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, show).apply();
  }

  public static boolean shouldShowEmulateBadStorageSetting()
  {
    return mPrefs.getBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, false);
  }

  /**
   * @param context context
   * @throws IOException if "Emulate bad storage" is enabled in preferences
   */
  public static void emulateBadExternalStorage(@NonNull Context context) throws IOException
  {
    final String key = context.getString(R.string.pref_emulate_bad_external_storage);
    if (mPrefs.getBoolean(key, false))
    {
      // Emulate one time only -> reset setting to run normally next time.
      mPrefs.edit().putBoolean(key, false).apply();
      throw new IOException("Bad external storage error injection");
    }
  }

  public static boolean shouldShowNewMarkerForLayerMode(@NonNull Mode mode)
  {
    return switch (mode)
    {
      case SUBWAY, TRAFFIC, ISOLINES -> false;
      default -> mPrefs.getBoolean(PREFS_SHOULD_SHOW_LAYER_MARKER_FOR + mode.name().toLowerCase(Locale.ENGLISH), true);
    };
  }

  public static void setLayerMarkerShownForLayerMode(@NonNull Mode mode)
  {
    mPrefs.edit()
        .putBoolean(PREFS_SHOULD_SHOW_LAYER_MARKER_FOR + mode.name().toLowerCase(Locale.ENGLISH), false)
        .apply();
  }
}
