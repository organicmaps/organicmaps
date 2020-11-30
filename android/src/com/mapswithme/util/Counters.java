package com.mapswithme.util;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.preference.PreferenceManager;
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
    updateLaunchCounter(context);
  }

  public static int getFirstInstallVersion(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getInt(KEY_APP_FIRST_INSTALL_VERSION, 0);
  }

  public static boolean isFirstStartDialogSeen(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, false);
  }

  public static void setFirstStartDialogSeen(@NonNull Context context)
  {
    MwmApplication.prefs(context)
                  .edit()
                  .putBoolean(KEY_MISC_FIRST_START_DIALOG_SEEN, true)
                  .apply();
  }


  public static void setWhatsNewShown(@NonNull Context context)
  {
    MwmApplication.prefs(context)
                  .edit()
                  .putInt(KEY_MISC_NEWS_LAST_VERSION, BuildConfig.VERSION_CODE)
                  .apply();
  }

  public static void resetAppSessionCounters(@NonNull Context context)
  {
    MwmApplication.prefs(context).edit()
                  .putInt(KEY_APP_LAUNCH_NUMBER, 0)
                  .putInt(KEY_APP_SESSION_NUMBER, 0)
                  .putLong(KEY_APP_LAST_SESSION_TIMESTAMP, 0L)
                  .putInt(KEY_LIKES_LAST_RATED_SESSION, 0)
                  .apply();
    incrementSessionNumber(context);
  }

  public static boolean isSessionRated(@NonNull Context context, int session)
  {
    return (MwmApplication.prefs(context).getInt(KEY_LIKES_LAST_RATED_SESSION,
                                                 0) >= session);
  }

  public static void setRatedSession(@NonNull Context context, int session)
  {
    MwmApplication.prefs(context).edit()
                  .putInt(KEY_LIKES_LAST_RATED_SESSION, session)
                  .apply();
  }

  /**
   * Session = single day, when app was started any number of times.
   */
  public static int getSessionCount(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getInt(KEY_APP_SESSION_NUMBER, 0);
  }

  public static boolean isRatingApplied(@NonNull Context context,
                                        Class<? extends DialogFragment> dialogFragmentClass)
  {
    return MwmApplication.prefs(context)
                         .getBoolean(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName(),
                                     false);
  }

  public static void setRatingApplied(@NonNull Context context,
                                      Class<? extends DialogFragment> dialogFragmentClass)
  {
    MwmApplication.prefs(context).edit()
                  .putBoolean(KEY_LIKES_RATED_DIALOG + dialogFragmentClass.getSimpleName(), true)
                  .apply();
  }

  public static String getInstallFlavor(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getString(KEY_APP_FIRST_INSTALL_FLAVOR, "");
  }

  private static void updateLaunchCounter(@NonNull Context context)
  {
    if (incrementLaunchNumber(context) == 0)
    {
      if (getFirstInstallVersion(context) == 0)
      {
        MwmApplication.prefs(context)
                      .edit()
                      .putInt(KEY_APP_FIRST_INSTALL_VERSION, BuildConfig.VERSION_CODE)
                      .apply();
      }

      updateInstallFlavor(context);
    }

    incrementSessionNumber(context);
  }

  private static int incrementLaunchNumber(@NonNull Context context)
  {
    return increment(context, KEY_APP_LAUNCH_NUMBER);
  }

  private static void updateInstallFlavor(@NonNull Context context)
  {
    String installedFlavor = getInstallFlavor(context);
    if (TextUtils.isEmpty(installedFlavor))
    {
      MwmApplication.prefs(context).edit()
                    .putString(KEY_APP_FIRST_INSTALL_FLAVOR, BuildConfig.FLAVOR)
                    .apply();
    }
  }

  private static void incrementSessionNumber(@NonNull Context context)
  {
    long lastSessionTimestamp = MwmApplication.prefs(context)
                                              .getLong(KEY_APP_LAST_SESSION_TIMESTAMP, 0);
    if (DateUtils.isToday(lastSessionTimestamp))
      return;

    MwmApplication.prefs(context).edit()
                  .putLong(KEY_APP_LAST_SESSION_TIMESTAMP, System.currentTimeMillis())
                  .apply();
    increment(context, KEY_APP_SESSION_NUMBER);
  }

  private static int increment(@NonNull Context context, @NonNull String key)
  {
    int value = MwmApplication.prefs(context).getInt(key, 0);
    MwmApplication.prefs(context).edit()
                  .putInt(key, ++value)
                  .apply();
    return value;
  }

  public static void setShowReviewForOldUser(@NonNull Context context, boolean value)
  {
    MwmApplication.prefs(context).edit()
                  .putBoolean(KEY_SHOW_REVIEW_FOR_OLD_USER, value)
                  .apply();
  }

  public static boolean isShowReviewForOldUser(@NonNull Context context)
  {
    return MwmApplication.prefs(context).getBoolean(KEY_SHOW_REVIEW_FOR_OLD_USER, false);
  }

  public static boolean isMigrationNeeded(@NonNull Context context)
  {
    return !MwmApplication.prefs(context).getBoolean(KEY_MIGRATION_EXECUTED, false);
  }

  public static void setMigrationExecuted(@NonNull Context context)
  {
    MwmApplication.prefs(context).edit()
                  .putBoolean(KEY_MIGRATION_EXECUTED, true)
                  .apply();
  }
}
