package app.organicmaps.sync.nc;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.Nullable;

import app.organicmaps.R;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

public final class NextcloudPreferences
{
  private static final String PREF_AUTHENTICATED_SERVER_URL = "authenticated_server_url";
  private static final String PREF_LOGIN_NAME = "login_name";
  private static final String PREF_APP_PASSWORD = "app_password";
  private static final String PREF_SYNC_ENABLED = "sync_enabled";

//  private static final String PREF_SYNC_INITIALIZED = "sync_initialized";  // this is set to false by default and also every time the user logs out
  private static final String PREF_CHANGED_FILES = "changed_files";
  // ETag preference keys follow the format ":${/file/path}" (e.g. ":/data/user/0/app.organicmaps.debug/files/bookmarks/My List.kml"

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
    NextcloudSyncer.INSTANCE.onLogout();
  }

//  public boolean getSyncInitialized()
//  {
//    return prefs.getBoolean(PREF_SYNC_INITIALIZED, false);
//  }
//
//  public void setSyncInitialized(boolean isInitialized)
//  {
//    SharedPreferences.Editor editor = prefs.edit();
//    editor.putBoolean(PREF_SYNC_INITIALIZED, isInitialized);
//  }

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
    if (enabled)
      NextcloudSyncer.INSTANCE.resumeSync();
    else
      NextcloudSyncer.INSTANCE.pauseSync();
  }

  public boolean getSyncEnabled()
  {
    return prefs.getBoolean(PREF_SYNC_ENABLED, false);
  }

  @Nullable
  public Set<String> getChangedFiles()
  {
    return prefs.getStringSet(PREF_CHANGED_FILES, null);
  }

  public void setChangedFiles(Set<String> changedFilepaths)
  {
    SharedPreferences.Editor editor = prefs.edit();
    editor.putStringSet(PREF_CHANGED_FILES, changedFilepaths);
    editor.apply();
  }
}