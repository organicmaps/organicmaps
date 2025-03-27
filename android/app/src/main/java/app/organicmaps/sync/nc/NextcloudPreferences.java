package app.organicmaps.sync.nc;

import android.content.Context;
import android.content.SharedPreferences;

import app.organicmaps.MwmApplication;

public final class NextcloudPreferences
{
  private static final String PREF_AUTHENTICATED_SERVER_URL = "pref_nc_authenticated_server_url";
  private static final String PREF_LOGIN_NAME = "pref_nc_login_name";
  private static final String PREF_APP_PASSWORD = "pref_nc_app_password";
  private static final String PREF_SYNC_ENABLED = "pref_nc_sync_enabled";

  public static boolean isAuthenticated(Context context)
  {
    SharedPreferences prefs = MwmApplication.prefs(context);
    String serverUrl = prefs.getString(PREF_AUTHENTICATED_SERVER_URL, null);
    String loginName = prefs.getString(PREF_LOGIN_NAME, null);
    String appPassword = prefs.getString(PREF_APP_PASSWORD, null);

    return serverUrl != null && !serverUrl.isEmpty()
        && loginName != null && !loginName.isEmpty()
        && appPassword != null && !appPassword.isEmpty();
  }

  public static void setAuthCredentials(Context context, String serverUrl, String loginName, String appPassword)
  {
    SharedPreferences.Editor editor = MwmApplication.prefs(context).edit();
    editor.putString(PREF_AUTHENTICATED_SERVER_URL, serverUrl);
    editor.putString(PREF_LOGIN_NAME, loginName);
    editor.putString(PREF_APP_PASSWORD, appPassword);
    editor.apply();
  }

  public static void clearAuthCredentials(Context context)
  {
    SharedPreferences.Editor editor = MwmApplication.prefs(context).edit();
    editor.remove(PREF_AUTHENTICATED_SERVER_URL);
    editor.remove(PREF_LOGIN_NAME);
    editor.remove(PREF_APP_PASSWORD);
    editor.apply();
  }

  public static String getAuthenticatedServerUrl(Context context)
  {
    return MwmApplication.prefs(context).getString(PREF_AUTHENTICATED_SERVER_URL, null);
  }

  public static String getLoginName(Context context)
  {
    return MwmApplication.prefs(context).getString(PREF_LOGIN_NAME, null);
  }

  public static String getAppPassword(Context context)
  {
    return MwmApplication.prefs(context).getString(PREF_APP_PASSWORD, null);
  }

  public static void setSyncEnabled(Context context, boolean enabled)
  {
    SharedPreferences.Editor editor = MwmApplication.prefs(context).edit();
    editor.putBoolean(PREF_SYNC_ENABLED, enabled);
    editor.apply();
  }

  public static boolean getSyncEnabled(Context context)
  {
    return MwmApplication.prefs(context).getBoolean(PREF_SYNC_ENABLED, false);
  }
}