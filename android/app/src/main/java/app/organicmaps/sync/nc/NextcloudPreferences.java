package app.organicmaps.sync.nc;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.R;

import java.util.Set;

public final class NextcloudPreferences
{
  public static final String PREF_SYNC_ENABLED = "sync_enabled";
  private final String PREF_AUTHENTICATED_SERVER_URL = "authenticated_server_url";
  private final String PREF_LOGIN_NAME = "login_name";
  private final String PREF_APP_PASSWORD = "app_password";
  private final String PREF_CHANGED_FILES = "changed_files";

  private final SharedPreferences prefs;
  private final SharedPreferences fileETagPrefs;

  public NextcloudPreferences(Context context)
  {
    prefs = context.getSharedPreferences(context.getString(R.string.pref_nc_sync_filename), Context.MODE_PRIVATE);
    fileETagPrefs = context.getSharedPreferences(context.getString(R.string.pref_nc_etags_filename), Context.MODE_PRIVATE);
  }

  public boolean isAuthenticated()
  {
    String serverUrl = prefs.getString(PREF_AUTHENTICATED_SERVER_URL, null);
    String loginName = prefs.getString(PREF_LOGIN_NAME, null);
    String appPassword = prefs.getString(PREF_APP_PASSWORD, null);

    return serverUrl != null && !serverUrl.isEmpty() && loginName != null && !loginName.isEmpty() && appPassword != null && !appPassword.isEmpty();
  }

  public void setAuthCredentials(String serverUrl, String loginName, String appPassword)
  {
    SharedPreferences.Editor editor = prefs.edit();
    editor.putString(PREF_AUTHENTICATED_SERVER_URL, serverUrl);
    editor.putString(PREF_LOGIN_NAME, loginName);
    editor.putString(PREF_APP_PASSWORD, appPassword);
    editor.apply();
  }

  /**
   * The only function that should be called to perform logout.
   */
  public void clearAuthCredentials()
  {
    setSyncEnabled(false);
    SharedPreferences.Editor editor = prefs.edit();
    editor.remove(PREF_AUTHENTICATED_SERVER_URL);
    editor.remove(PREF_LOGIN_NAME);
    editor.remove(PREF_APP_PASSWORD);
    editor.apply();
    NextcloudSyncer.INSTANCE.onLogout();
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

  public boolean getSyncEnabled()
  {
    return prefs.getBoolean(PREF_SYNC_ENABLED, false);
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

  public void clearFileETags()
  {
    fileETagPrefs.edit().clear().apply();
  }

  /**
   * Shortens the file path into a pref key (this implementation can't be (easily?) changed once (if ever) released into production)
   */
  private String getPrefKeyFromFilePath(String filepath)
  {
    String[] splitPath = filepath.split("/");
    return splitPath[splitPath.length - 1];
  }

  public void setFileETag(String filepath, @NonNull String eTag)
  {
    fileETagPrefs.edit().putString(getPrefKeyFromFilePath(filepath), eTag).apply();
  }

  public void removeFileETag(String filepath)
  {
    fileETagPrefs.edit().remove(getPrefKeyFromFilePath(filepath)).apply();
  }

  @Nullable
  public String getFileETag(String filepath)
  {
    return fileETagPrefs.getString(getPrefKeyFromFilePath(filepath), null);
  }
}