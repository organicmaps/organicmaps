package app.organicmaps.sdk.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.R;

public final class Config
{
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static SharedPreferences mPrefs;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static String mFlavor;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static String mApplicationId;

  private static int mVersionCode;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static String mVersionName;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static String mFileProviderAuthority;

  private static final String KEY_APP_STORAGE = "StoragePath";

  private static final String KEY_DOWNLOADER_AUTO = "AutoDownloadEnabled";
  private static final String KEY_PREF_ZOOM_BUTTONS = "ZoomButtonsEnabled";
  static final String KEY_PREF_STATISTICS = "StatisticsEnabled";
  private static final String KEY_PREF_USE_GS = "UseGoogleServices";

  private static final String KEY_MISC_DISCLAIMER_ACCEPTED = "IsDisclaimerApproved";

  private static final String KEY_MISC_LOCATION_REQUESTED = "LocationRequested";
  private static final String KEY_MISC_USE_MOBILE_DATA = "UseMobileData";
  private static final String KEY_MISC_USE_MOBILE_DATA_TIMESTAMP = "UseMobileDataTimestamp";
  private static final String KEY_MISC_USE_MOBILE_DATA_ROAMING = "UseMobileDataRoaming";
  private static final String KEY_MISC_KEEP_SCREEN_ON = "KeepScreenOn";

  private static final String KEY_MISC_SHOW_ON_LOCK_SCREEN = "ShowOnLockScreen";
  private static final String KEY_MISC_AGPS_TIMESTAMP = "AGPSTimestamp";
  private static final String KEY_DONATE_URL = "DonateUrl";
  private static final String KEY_PREF_SEARCH_HISTORY = "SearchHistoryEnabled";

  public static final String KEY_PREF_LAST_SEARCHED_TAB = "LastSearchTab";

  /**
   * The total number of app launches.
   */
  private static final String KEY_APP_LAUNCH_NUMBER = "LaunchNumber";
  /**
   * The timestamp for the most recent app launch.
   */
  private static final String KEY_APP_LAST_SESSION_TIMESTAMP = "LastSessionTimestamp";
  /**
   * The version code of the first installed version of the app.
   */
  private static final String KEY_APP_FIRST_INSTALL_VERSION_CODE = "FirstInstallVersion";
  /**
   * The version code of the last launched version of the app.
   */
  private static final String KEY_APP_LAST_INSTALL_VERSION_CODE = "LastInstallVersion";
  /**
   * True if the first start animation has been seen.
   */
  private static final String KEY_MISC_FIRST_START_DIALOG_SEEN = "FirstStartDialogSeen";

  private Config() {}

  private static boolean isFdroid()
  {
    return mFlavor.equals("fdroid");
  }

  private static int getInt(String key, int def)
  {
    return nativeGetInt(key, def);
  }

  private static long getLong(String key, long def)
  {
    return nativeGetLong(key, def);
  }

  private static float getFloat(@NonNull final String key, final float def)
  {
    return (float) nativeGetDouble(key, def);
  }

  @NonNull
  private static String getString(String key)
  {
    return getString(key, "");
  }

  @NonNull
  private static String getString(String key, String def)
  {
    return nativeGetString(key, def);
  }

  private static boolean getBool(String key)
  {
    return getBool(key, false);
  }

  private static boolean getBool(String key, boolean def)
  {
    return nativeGetBoolean(key, def);
  }

  private static void setInt(String key, int value)
  {
    nativeSetInt(key, value);
  }

  private static void setLong(String key, long value)
  {
    nativeSetLong(key, value);
  }

  private static void setFloat(@NonNull final String key, final float value)
  {
    nativeSetDouble(key, value);
  }

  private static void setString(String key, String value)
  {
    nativeSetString(key, value);
  }

  private static void setBool(String key)
  {
    setBool(key, true);
  }

  private static void setBool(String key, boolean value)
  {
    nativeSetBoolean(key, value);
  }

  @NonNull
  public static String getApplicationId()
  {
    return mApplicationId;
  }

  public static int getVersionCode()
  {
    return mVersionCode;
  }

  @NonNull
  public static String getVersionName()
  {
    return mVersionName;
  }

  @NonNull
  public static String getFileProviderAuthority()
  {
    return mFileProviderAuthority;
  }

  public static String getStoragePath()
  {
    return getString(KEY_APP_STORAGE);
  }

  public static void setStoragePath(String path)
  {
    setString(KEY_APP_STORAGE, path);
  }

  public static boolean isAutodownloadEnabled()
  {
    return getBool(KEY_DOWNLOADER_AUTO, true);
  }

  public static void setAutodownloadEnabled(boolean enabled)
  {
    setBool(KEY_DOWNLOADER_AUTO, enabled);
  }

  public static boolean showZoomButtons()
  {
    return getBool(KEY_PREF_ZOOM_BUTTONS, true);
  }

  public static void setShowZoomButtons(boolean show)
  {
    setBool(KEY_PREF_ZOOM_BUTTONS, show);
  }

  public static void setStatisticsEnabled(boolean enabled)
  {
    setBool(KEY_PREF_STATISTICS, enabled);
  }

  public static boolean isKeepScreenOnEnabled()
  {
    return getBool(KEY_MISC_KEEP_SCREEN_ON, false);
  }

  public static void setKeepScreenOnEnabled(boolean enabled)
  {
    setBool(KEY_MISC_KEEP_SCREEN_ON, enabled);
  }

  public static boolean isShowOnLockScreenEnabled()
  {
    // Disabled by default on Android 7.1 and earlier devices.
    // See links below for details:
    // https://github.com/organicmaps/organicmaps/issues/2857
    // https://github.com/organicmaps/organicmaps/issues/3967
    final boolean defaultValue = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O;
    return getBool(KEY_MISC_SHOW_ON_LOCK_SCREEN, defaultValue);
  }

  public static void setShowOnLockScreenEnabled(boolean enabled)
  {
    setBool(KEY_MISC_SHOW_ON_LOCK_SCREEN, enabled);
  }

  public static boolean useGoogleServices()
  {
    // F-droid users expect non-free networks to be disabled by default
    // https://t.me/organicmaps/47334
    // Additionally, in the µG play-services-location library which is used for
    // F-droid builds, GMS api availability is stubbed and always returns true.
    // https://github.com/microg/GmsCore/issues/2309
    // For more details, see the discussion in
    // https://github.com/organicmaps/organicmaps/pull/9575
    return getBool(KEY_PREF_USE_GS, !isFdroid());
  }

  public static void setUseGoogleService(boolean use)
  {
    setBool(KEY_PREF_USE_GS, use);
  }

  public static boolean isRoutingDisclaimerAccepted()
  {
    return getBool(KEY_MISC_DISCLAIMER_ACCEPTED);
  }

  public static void acceptRoutingDisclaimer()
  {
    setBool(KEY_MISC_DISCLAIMER_ACCEPTED);
  }

  public static boolean isLocationRequested()
  {
    return getBool(KEY_MISC_LOCATION_REQUESTED);
  }

  public static void setLocationRequested()
  {
    setBool(KEY_MISC_LOCATION_REQUESTED);
  }

  public static class UiTheme
  {
    private static final String KEY_UI_THEME = "UiTheme";
    private static final String KEY_UI_THEME_SETTINGS = "UiThemeSettings";

    public static final String AUTO = "auto";
    public static final String NIGHT = "night";
    public static final String NAV_AUTO = "nav_auto";
    public static final String DEFAULT = "default";

    public static boolean isAuto(@NonNull String theme)
    {
      return AUTO.equals(theme);
    }

    public static boolean isNavAuto(@NonNull String theme)
    {
      return NAV_AUTO.equals(theme);
    }

    public static boolean isNight(@NonNull String theme)
    {
      return NIGHT.equals(theme);
    }

    public static boolean isDefault(@NonNull String theme)
    {
      return DEFAULT.equals(theme);
    }

    @NonNull
    public static String getCurrent()
    {
      final String res = getString(KEY_UI_THEME, DEFAULT);
      if (isValid(res))
        return res;

      return DEFAULT;
    }

    public static void setCurrent(@NonNull String theme)
    {
      if (getCurrent().equals(theme))
        return;

      setString(KEY_UI_THEME, theme);
    }

    @NonNull
    public static String getUiThemeSettings()
    {
      final String res = getString(KEY_UI_THEME_SETTINGS, DEFAULT);
      if (isValid(res) || isAuto(res) || isNavAuto(res))
        return res;

      return DEFAULT;
    }

    public static boolean setUiThemeSettings(String theme)
    {
      if (getUiThemeSettings().equals(theme))
        return false;

      setString(KEY_UI_THEME_SETTINGS, theme);
      return true;
    }

    private static boolean isValid(@NonNull String theme)
    {
      return DEFAULT.equals(theme) || NIGHT.equals(theme);
    }
  }

  public static boolean isLargeFontsSize()
  {
    return nativeGetLargeFontsSize();
  }

  public static void setLargeFontsSize(boolean value)
  {
    nativeSetLargeFontsSize(value);
  }

  @NonNull
  public static NetworkPolicy.Type getUseMobileDataSettings()
  {
    int value = getInt(KEY_MISC_USE_MOBILE_DATA, NetworkPolicy.NONE);

    if (value < 0 || value >= NetworkPolicy.Type.values().length)
      return NetworkPolicy.Type.ASK;

    return NetworkPolicy.Type.values()[value];
  }

  public static void setUseMobileDataSettings(@NonNull NetworkPolicy.Type value)
  {
    setInt(KEY_MISC_USE_MOBILE_DATA, value.ordinal());
    setBool(KEY_MISC_USE_MOBILE_DATA_ROAMING, ConnectionState.INSTANCE.isInRoaming());
  }

  public static void setMobileDataTimeStamp(long timestamp)
  {
    setLong(KEY_MISC_USE_MOBILE_DATA_TIMESTAMP, timestamp);
  }

  static long getMobileDataTimeStamp()
  {
    return getLong(KEY_MISC_USE_MOBILE_DATA_TIMESTAMP, 0L);
  }

  static boolean getMobileDataRoaming()
  {
    return getBool(KEY_MISC_USE_MOBILE_DATA_ROAMING, false);
  }

  public static void setAgpsTimestamp(long timestamp)
  {
    setLong(KEY_MISC_AGPS_TIMESTAMP, timestamp);
  }

  public static long getAgpsTimestamp()
  {
    return getLong(KEY_MISC_AGPS_TIMESTAMP, 0L);
  }

  public static boolean isTransliteration()
  {
    return nativeGetTransliteration();
  }

  public static void setTransliteration(boolean value)
  {
    nativeSetTransliteration(value);
  }

  public static boolean isNY()
  {
    return getBool("NY");
  }

  @NonNull
  public static String getDonateUrl()
  {
    return getString(KEY_DONATE_URL);
  }

  public static void init(@NonNull Context context, @NonNull SharedPreferences prefs, @NonNull String flavor,
                          @NonNull String applicationId, int versionCode, @NonNull String versionName,
                          @NonNull String fileProviderAuthority)
  {
    mPrefs = prefs;
    mFlavor = flavor;
    mApplicationId = applicationId;
    mVersionCode = versionCode;
    mVersionName = versionName;
    mFileProviderAuthority = fileProviderAuthority;
    final SharedPreferences.Editor editor = mPrefs.edit();

    // Update counters.
    final int launchNumber = mPrefs.getInt(KEY_APP_LAUNCH_NUMBER, 0);
    editor.putInt(KEY_APP_LAUNCH_NUMBER, launchNumber + 1);
    editor.putLong(KEY_APP_LAST_SESSION_TIMESTAMP, System.currentTimeMillis());
    editor.putInt(KEY_APP_LAST_INSTALL_VERSION_CODE, mVersionCode);
    if (launchNumber == 0 || mPrefs.getInt(KEY_APP_FIRST_INSTALL_VERSION_CODE, 0) == 0)
      editor.putInt(KEY_APP_FIRST_INSTALL_VERSION_CODE, mVersionCode);

    // Clean up legacy counters.
    editor.remove("FirstInstallFlavor");
    editor.remove("SessionNumber");
    editor.remove("WhatsNewShownVersion");
    editor.remove("LastRatedSession");
    editor.remove("RatedDialog");

    // Migrate ENABLE_SCREEN_SLEEP to KEEP_SCREEN_ON.
    final String KEY_MISC_ENABLE_SCREEN_SLEEP = "EnableScreenSleep";
    if (nativeHasConfigValue(KEY_MISC_ENABLE_SCREEN_SLEEP))
    {
      nativeSetBoolean(KEY_MISC_KEEP_SCREEN_ON, !getBool(KEY_MISC_ENABLE_SCREEN_SLEEP, false));
      nativeDeleteConfigValue(KEY_MISC_ENABLE_SCREEN_SLEEP);
    }

    editor.apply();
  }

  public static boolean isFirstLaunch(@NonNull Context context)
  {
    return !mPrefs.getBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, false);
  }

  public static void setFirstStartDialogSeen(@NonNull Context context)
  {
    mPrefs.edit().putBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, true).apply();
  }

  public static boolean isSearchHistoryEnabled()
  {
    return getBool(KEY_PREF_SEARCH_HISTORY, true);
  }

  public static void setSearchHistoryEnabled(boolean enabled)
  {
    setBool(KEY_PREF_SEARCH_HISTORY, enabled);
  }

  public static class TTS
  {
    interface Keys
    {
      String ENABLED = "TtsEnabled";
      String LANGUAGE = "TtsLanguage";
      String VOLUME = "TtsVolume";
      String STREETS = "TtsStreetNames";
    }

    public interface Defaults
    {
      boolean ENABLED = true;

      float VOLUME_MIN = 0.0f;
      float VOLUME_MAX = 1.0f;
      float VOLUME = VOLUME_MAX;

      boolean STREETS = false; // TTS may mangle some languages, do not announce streets by default
    }

    public static boolean isEnabled()
    {
      return getBool(Keys.ENABLED, Defaults.ENABLED);
    }

    public static void setEnabled(final boolean enabled)
    {
      setBool(Keys.ENABLED, enabled);
    }

    @NonNull
    public static String getLanguage()
    {
      return getString(Keys.LANGUAGE);
    }

    public static void setLanguage(@NonNull final String language)
    {
      setString(Keys.LANGUAGE, language);
    }

    public static float getVolume()
    {
      return getFloat(Keys.VOLUME, Defaults.VOLUME);
    }

    public static void setVolume(final float volume)
    {
      setFloat(Keys.VOLUME, volume);
    }

    public static boolean getAnnounceStreets()
    {
      return getBool(Keys.STREETS, Defaults.STREETS);
    }

    public static void setAnnounceStreets(boolean enabled)
    {
      setBool(Keys.STREETS, enabled);
    }
  }

  private static native boolean nativeHasConfigValue(String name);
  private static native boolean nativeDeleteConfigValue(String name);
  private static native boolean nativeGetBoolean(String name, boolean defaultValue);
  private static native void nativeSetBoolean(String name, boolean value);
  private static native int nativeGetInt(String name, int defaultValue);
  private static native void nativeSetInt(String name, int value);
  private static native long nativeGetLong(String name, long defaultValue);
  private static native void nativeSetLong(String name, long value);
  private static native double nativeGetDouble(String name, double defaultValue);
  private static native void nativeSetDouble(String name, double value);
  private static native String nativeGetString(String name, String defaultValue);
  private static native void nativeSetString(String name, String value);
  private static native boolean nativeGetLargeFontsSize();
  private static native void nativeSetLargeFontsSize(boolean value);
  private static native boolean nativeGetTransliteration();
  private static native void nativeSetTransliteration(boolean value);
}
