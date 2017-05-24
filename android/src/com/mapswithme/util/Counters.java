package com.mapswithme.util;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v7.preference.PreferenceManager;
import android.text.TextUtils;
import android.text.format.DateUtils;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public final class Counters
{
  static final String KEY_APP_LAUNCH_NUMBER = "LaunchNumber";
  static final String KEY_APP_FIRST_INSTALL_VERSION = "FirstInstallVersion";
  static final String KEY_APP_FIRST_INSTALL_FLAVOR = "FirstInstallFlavor";
  static final String KEY_APP_LAST_SESSION_TIMESTAMP = "LastSessionTimestamp";
  static final String KEY_APP_SESSION_NUMBER = "SessionNumber";
  static final String KEY_MISC_FIRST_START_DIALOG_SEEN = "FirstStartDialogSeen";
  static final String KEY_MISC_NEWS_LAST_VERSION = "WhatsNewShownVersion";
  static final String KEY_LIKES_LAST_RATED_SESSION = "LastRatedSession";

  private static final String KEY_LIKES_RATED_DIALOG = "RatedDialog";
  private static final String KEY_SHOW_REVIEW_FOR_OLD_USER = "ShowReviewForOldUser";
  private static final String KEY_MIGRATION_EXECUTED = "MigrationExecuted";

  private Counters() {}

  public static void initCounters(@NonNull Context context)
  {
    PreferenceManager.setDefaultValues(context, R.xml.prefs_main, false);
    updateLaunchCounter();
  }

  public static int getFirstInstallVersion()
  {
    return MwmApplication.prefs().getInt(KEY_APP_FIRST_INSTALL_VERSION, 0);
  }

  public static boolean isFirstStartDialogSeen()
  {
    return MwmApplication.prefs().getBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, false);
  }

  public static void setFirstStartDialogSeen()
  {
    MwmApplication.prefs()
                  .edit()
                  .putBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, true)
                  .apply();
  }

  public static int getLastWhatsNewVersion()
  {
    return MwmApplication.prefs().getInt(KEY_MISC_NEWS_LAST_VERSION, 0);
  }

  public static void setWhatsNewShown()
  {
    MwmApplication.prefs()
                  .edit()
                  .putInt(KEY_MISC_NEWS_LAST_VERSION, BuildConfig.VERSION_CODE)
                  .apply();
  }

  public static void resetAppSessionCounters()
  {
    MwmApplication.prefs()
                  .edit()
                  .putInt(KEY_APP_LAUNCH_NUMBER, 0)
                  .putInt(KEY_APP_SESSION_NUMBER, 0)
                  .putLong(KEY_APP_LAST_SESSION_TIMESTAMP, 0L)
                  .putInt(KEY_LIKES_LAST_RATED_SESSION, 0)
                  .apply();
    incrementSessionNumber();
  }

  public static boolean isSessionRated(int session)
  {
    return (MwmApplication.prefs().getInt(KEY_LIKES_LAST_RATED_SESSION, 0) >= session);
  }

  public static void setRatedSession(int session)
  {
    MwmApplication.prefs()
                  .edit()
                  .putInt(KEY_LIKES_LAST_RATED_SESSION, session)
                  .apply();
  }

  /**
   * Session = single day, when app was started any number of times.
   */
  public static int getSessionCount()
  {
    return MwmApplication.prefs().getInt(KEY_APP_SESSION_NUMBER, 0);
  }

  public static boolean isRatingApplied(Class<? extends DialogFragment> dialogFragmentClass)
  {
    return MwmApplication.prefs()
                         .getBoolean(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName(),
                                     false);
  }

  public static void setRatingApplied(Class<? extends DialogFragment> dialogFragmentClass)
  {
    MwmApplication.prefs()
                  .edit()
                  .putBoolean(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName(), true)
                  .apply();
  }

  public static String getInstallFlavor()
  {
    return MwmApplication.prefs().getString(KEY_APP_FIRST_INSTALL_FLAVOR, "");
  }

  private static void updateLaunchCounter()
  {
    if (incrementLaunchNumber() == 0)
    {
      if (getFirstInstallVersion() == 0)
      {
        MwmApplication.prefs()
                      .edit()
                      .putInt(KEY_APP_FIRST_INSTALL_VERSION, BuildConfig.VERSION_CODE)
                      .apply();
      }

      updateInstallFlavor();
    }

    incrementSessionNumber();
  }

  private static int incrementLaunchNumber()
  {
    return increment(KEY_APP_LAUNCH_NUMBER);
  }

  private static void updateInstallFlavor()
  {
    String installedFlavor = getInstallFlavor();
    if (TextUtils.isEmpty(installedFlavor))
    {
      MwmApplication.prefs()
                    .edit()
                    .putString(KEY_APP_FIRST_INSTALL_FLAVOR, BuildConfig.FLAVOR)
                    .apply();
    }
  }

  private static void incrementSessionNumber()
  {
    long lastSessionTimestamp = MwmApplication.prefs().getLong(KEY_APP_LAST_SESSION_TIMESTAMP, 0);
    if (DateUtils.isToday(lastSessionTimestamp))
      return;

    MwmApplication.prefs()
                  .edit()
                  .putLong(KEY_APP_LAST_SESSION_TIMESTAMP, System.currentTimeMillis())
                  .apply();
    increment(KEY_APP_SESSION_NUMBER);
  }

  private static int increment(@NonNull String key)
  {
    int value = MwmApplication.prefs().getInt(key, 0);
    MwmApplication.prefs()
                  .edit()
                  .putInt(key, ++value)
                  .apply();
    return value;
  }

  public static void setShowReviewForOldUser(boolean value)
  {
    MwmApplication.prefs()
                  .edit()
                  .putBoolean(KEY_SHOW_REVIEW_FOR_OLD_USER, value)
                  .apply();
  }

  public static boolean isShowReviewForOldUser()
  {
    return MwmApplication.prefs().getBoolean(KEY_SHOW_REVIEW_FOR_OLD_USER, false);
  }

  public static boolean isMigrationNeeded()
  {
    return !MwmApplication.prefs().getBoolean(KEY_MIGRATION_EXECUTED, false);
  }

  public static void setMigrationExecuted()
  {
    MwmApplication.prefs()
                  .edit()
                  .putBoolean(KEY_MIGRATION_EXECUTED, true)
                  .apply();
  }
}
