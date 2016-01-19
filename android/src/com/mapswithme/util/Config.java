package com.mapswithme.util;

import android.support.v4.app.DialogFragment;
import android.text.TextUtils;
import android.text.format.DateUtils;

import com.mapswithme.maps.BuildConfig;

public final class Config
{
  private static final String KEY_APP_FIRST_INSTALL_VERSION = "FirstInstallVersion";
  private static final String KEY_APP_LAUNCH_NUMBER = "LaunchNumber";
  private static final String KEY_APP_SESSION_NUMBER = "SessionNumber";
  private static final String KEY_APP_LAST_SESSION_TIMESTAMP = "LastSessionTimestamp";
  private static final String KEY_APP_FIRST_INSTALL_FLAVOR = "FirstInstallFlavor";

  private static final String KEY_TTS_ENABLED = "TtsEnabled";
  private static final String KEY_TTS_LANGUAGE = "TtsLanguage";

  private static final String KEY_PREF_ZOOM_BUTTONS = "ZoomButtonsEnabled";
  private static final String KEY_PREF_STATISTICS = "StatisticsEnabled";

  private static final String KEY_LIKES_RATED_DIALOG = "RatedDialog";
  private static final String KEY_LIKES_LAST_RATED_SESSION = "LastRatedSession";

  private static final String KEY_MISC_DISCLAIMER_ACCEPTED = "IsDisclaimerApproved";
  private static final String KEY_MISC_KITKAT_MIGRATED = "KitKatMigrationCompleted";
  private static final String KEY_MISC_NEWS_LAST_VERSION = "WhatsNewShownVersion";
  private static final String KEY_MISC_UI_THEME = "UiTheme";
  private static final String KEY_MISC_UI_THEME_SETTINGS = "UiThemeSettings";

  private Config() {}

  private static int getInt(String key)
  {
    return getInt(key, 0);
  }

  private static int getInt(String key, int def)
  {
    return nativeGetInt(key, def);
  }

  private static long getLong(String key)
  {
    return getLong(key, 0L);
  }

  private static long getLong(String key, long def)
  {
    return nativeGetLong(key, def);
  }

  private static String getString(String key)
  {
    return getString(key, "");
  }

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

  /**
   * Increments integer value.
   * @return Previous value before increment.
   */
  private static int increment(String key)
  {
    int res = getInt(key);
    setInt(key, res + 1);
    return res;
  }

  public static int getFirstInstallVersion()
  {
    return getInt(KEY_APP_FIRST_INSTALL_VERSION);
  }

  /**
   * Increments counter of app starts.
   * @return Previous value before increment.
   */
  public static int incrementLaunchNumber()
  {
    return increment(KEY_APP_LAUNCH_NUMBER);
  }

  /**
   * Session = single day, when app was started any number of times.
   */
  public static int getSessionCount()
  {
    return getInt(KEY_APP_SESSION_NUMBER);
  }

  private static void incrementSessionNumber()
  {
    long lastSessionTimestamp = getLong(KEY_APP_LAST_SESSION_TIMESTAMP);
    if (DateUtils.isToday(lastSessionTimestamp))
      return;

    setLong(KEY_APP_LAST_SESSION_TIMESTAMP, System.currentTimeMillis());
    increment(KEY_APP_SESSION_NUMBER);
  }

  public static void resetAppSessionCounters()
  {
    setInt(KEY_APP_LAUNCH_NUMBER, 0);
    setInt(KEY_APP_SESSION_NUMBER, 0);
    setLong(KEY_APP_LAST_SESSION_TIMESTAMP, 0L);
    setInt(KEY_LIKES_LAST_RATED_SESSION, 0);
    incrementSessionNumber();
  }

  public static String getInstallFlavor()
  {
    return getString(KEY_APP_FIRST_INSTALL_FLAVOR);
  }

  private static void updateInstallFlavor()
  {
    String installedFlavor = getInstallFlavor();
    if (TextUtils.isEmpty(installedFlavor))
      setString(KEY_APP_FIRST_INSTALL_FLAVOR, BuildConfig.FLAVOR);
  }

  public static void updateLaunchCounter()
  {
    if (incrementLaunchNumber() == 0)
    {
      if (getFirstInstallVersion() == 0)
        setInt(KEY_APP_FIRST_INSTALL_VERSION, BuildConfig.VERSION_CODE);

      updateInstallFlavor();
    }

    incrementSessionNumber();
  }

  public static boolean isTtsEnabled()
  {
    return getBool(KEY_TTS_ENABLED, true);
  }

  public static void setTtsEnabled(boolean enabled)
  {
    setBool(KEY_TTS_ENABLED, enabled);
  }

  public static String getTtsLanguage()
  {
    return getString(KEY_TTS_LANGUAGE);
  }

  public static void setTtsLanguage(String language)
  {
    setString(KEY_TTS_LANGUAGE, language);
  }

  public static boolean showZoomButtons()
  {
    return getBool(KEY_PREF_ZOOM_BUTTONS, true);
  }

  public static void setShowZoomButtons(boolean show)
  {
    setBool(KEY_PREF_ZOOM_BUTTONS, show);
  }

  public static boolean isStatisticsEnabled()
  {
    return getBool(KEY_PREF_STATISTICS, true);
  }

  public static void setStatisticsEnabled(boolean enabled)
  {
    setBool(KEY_PREF_STATISTICS, enabled);
  }

  public static boolean isRatingApplied(Class<? extends DialogFragment> dialogFragmentClass)
  {
    return getBool(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName());
  }

  public static void setRatingApplied(Class<? extends DialogFragment> dialogFragmentClass)
  {
    setBool(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName(), true);
  }

  public static boolean isSessionRated(int session)
  {
    return (getInt(KEY_LIKES_LAST_RATED_SESSION) >= session);
  }

  public static void setRatedSession(int session)
  {
    setInt(KEY_LIKES_LAST_RATED_SESSION, session);
  }

  public static boolean isRoutingDisclaimerAccepted()
  {
    return getBool(KEY_MISC_DISCLAIMER_ACCEPTED);
  }

  public static void acceptRoutingDisclaimer()
  {
    setBool(KEY_MISC_DISCLAIMER_ACCEPTED, true);
  }

  public static boolean isKitKatMigrationComplete()
  {
    return getBool(KEY_MISC_KITKAT_MIGRATED);
  }

  public static void setKitKatMigrationComplete()
  {
    setBool(KEY_MISC_KITKAT_MIGRATED);
  }

  public static int getLastWhatsNewVersion()
  {
    return getInt(KEY_MISC_NEWS_LAST_VERSION);
  }

  public static void setWhatsNewShown()
  {
    setInt(KEY_MISC_NEWS_LAST_VERSION, BuildConfig.VERSION_CODE);
  }

  public static String getCurrentUiTheme()
  {
    String res = getString(KEY_MISC_UI_THEME, ThemeUtils.THEME_DEFAULT);
    if (ThemeUtils.isValidTheme(res))
      return res;

    return ThemeUtils.THEME_DEFAULT;
  }

  public static void setCurrentUiTheme(String theme)
  {
    if (getCurrentUiTheme().equals(theme))
      return;

    setString(KEY_MISC_UI_THEME, theme);
    ThemeSwitcher.changeMapStyle(theme);
  }

  public static String getUiThemeSettings()
  {
    String res = getString(KEY_MISC_UI_THEME_SETTINGS, ThemeUtils.THEME_AUTO);
    if (ThemeUtils.isValidTheme(res) || ThemeUtils.isAutoTheme(res))
      return res;

    return ThemeUtils.THEME_AUTO;
  }

  public static boolean setUiThemeSettings(String theme)
  {
    if (getUiThemeSettings().equals(theme))
      return false;

    setString(KEY_MISC_UI_THEME_SETTINGS, theme);
    return true;
  }

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
}
