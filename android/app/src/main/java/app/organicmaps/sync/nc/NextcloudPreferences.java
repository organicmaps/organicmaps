package app.organicmaps.sync.nc;

import android.content.Context;
import android.content.SharedPreferences;

import app.organicmaps.MwmApplication;
import app.organicmaps.R;

public final class NextcloudPreferences
{
  private static final String PREF_AUTHENTICATED_SERVER_URL = "pref_nc_authenticated_server_url";
  private static final String PREF_LOGIN_NAME = "pref_nc_login_name";
  private static final String PREF_APP_PASSWORD = "pref_nc_app_password";
  private static final String PREF_SYNC_ENABLED = "pref_nc_sync_enabled";

  private final SharedPreferences prefs;

  public NextcloudPreferences(Context context)
  {
    prefs = context.getSharedPreferences(context.getString(R.string.pref_nc_sync_filename),Context.MODE_PRIVATE);
  }

  public boolean isAuthenticated()
  {
    String serverUrl = prefs.getString(PREF_AUTHENTICATED_SERVER_URL, null);
    String loginName = prefs.getString(PREF_LOGIN_NAME, null);
    String appPassword = prefs.getString(PREF_APP_PASSWORD, null);

    return serverUrl != null && !serverUrl.isEmpty()
        && loginName != null && !loginName.isEmpty()
        && appPassword != null && !appPassword.isEmpty();
  }

  public void setAuthCredentials(String serverUrl, String loginName, String appPassword)
  {
    SharedPreferences.Editor editor = prefs.edit();
    editor.putString(PREF_AUTHENTICATED_SERVER_URL, serverUrl);
    editor.putString(PREF_LOGIN_NAME, loginName);
    editor.putString(PREF_APP_PASSWORD, appPassword);
    editor.apply();
  }

  public void clearAuthCredentials()
  {
    SharedPreferences.Editor editor = prefs.edit();
    editor.remove(PREF_AUTHENTICATED_SERVER_URL);
    editor.remove(PREF_LOGIN_NAME);
    editor.remove(PREF_APP_PASSWORD);
    editor.apply();
  }

  public String getAuthenticatedServerUrl()
  {
    return prefs.getString(PREF_AUTHENTICATED_SERVER_URL, null);
  }

  public String getLoginName()
  {
    return prefs.getString(PREF_LOGIN_NAME, null);
  }

  public String getAppPassword()
  {
    return prefs.getString(PREF_APP_PASSWORD, null);
  }

  public void setSyncEnabled(boolean enabled)
  {
    SharedPreferences.Editor editor = prefs.edit();
    editor.putBoolean(PREF_SYNC_ENABLED, enabled);
    editor.apply();
  }

  public boolean getSyncEnabled()
  {
    return prefs.getBoolean(PREF_SYNC_ENABLED, false);
  }
}