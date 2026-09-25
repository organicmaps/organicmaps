package app.organicmaps.bookmarks.cloud;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.sdk.bookmarks.data.FileType;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Orchestrates automatic and manual synchronization of Organic Maps bookmarks with Nextcloud.
 */
public enum NextcloudSyncManager implements BookmarkManager.BookmarksSharingListener
{
  INSTANCE;

  private static final String TAG = NextcloudSyncManager.class.getSimpleName();
  private static final String PREFS_NAME = "organicmaps_nextcloud_sync";
  private static final String KEY_SERVER_URL = "server_url";
  private static final String KEY_USERNAME = "username";
  private static final String KEY_APP_TOKEN = "app_token";
  private static final String KEY_AUTO_SYNC = "auto_sync_enabled";
  private static final String KEY_LAST_SYNC_TIMESTAMP = "last_sync_timestamp";

  private final ExecutorService mExecutor = Executors.newSingleThreadExecutor();

  public interface SyncCallback
  {
    void onSuccess(String message);
    void onFailure(String errorMessage);
  }

  public boolean isConfigured(@NonNull Context context)
  {
    SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    return prefs.contains(KEY_SERVER_URL) && prefs.contains(KEY_USERNAME) && prefs.contains(KEY_APP_TOKEN);
  }

  public void saveCredentials(@NonNull Context context, @NonNull String serverUrl,
                              @NonNull String username, @NonNull String appToken, boolean autoSync)
  {
    context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        .edit()
        .putString(KEY_SERVER_URL, serverUrl)
        .putString(KEY_USERNAME, username)
        .putString(KEY_APP_TOKEN, appToken)
        .putBoolean(KEY_AUTO_SYNC, autoSync)
        .apply();
  }

  @Nullable
  public NextcloudWebDavClient getClient(@NonNull Context context)
  {
    SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    String url = prefs.getString(KEY_SERVER_URL, null);
    String user = prefs.getString(KEY_USERNAME, null);
    String token = prefs.getString(KEY_APP_TOKEN, null);

    if (url == null || user == null || token == null)
      return null;

    return new NextcloudWebDavClient(url, user, token);
  }

  public void startBackup(@NonNull Context context, @Nullable SyncCallback callback)
  {
    NextcloudWebDavClient client = getClient(context);
    if (client == null)
    {
      if (callback != null)
        callback.onFailure("Nextcloud credentials not configured.");
      return;
    }

    mExecutor.execute(() -> {
      List<BookmarkCategory> categories = BookmarkManager.INSTANCE.getCategories();
      if (categories.isEmpty())
      {
        if (callback != null)
          callback.onFailure("No bookmarks available to backup.");
        return;
      }

      long[] catIds = new long[categories.size()];
      for (int i = 0; i < categories.size(); i++)
        catIds[i] = categories.get(i).getId();

      BookmarkManager.INSTANCE.addSharingListener(new BookmarkManager.BookmarksSharingListener()
      {
        @Override
        public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
        {
          BookmarkManager.INSTANCE.removeSharingListener(this);
          if (result.getCode() == BookmarkSharingResult.SUCCESS && result.getSharingPath() != null)
          {
            File localArchive = new File(result.getSharingPath());
            String dateSuffix = new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(new Date());
            String remoteName = "OrganicMaps_Bookmarks_" + dateSuffix + ".kmz";

            boolean uploaded = client.uploadBookmarkFile(localArchive, remoteName);
            // Also keep a canonical 'latest.kmz' for quick one-click restore
            if (uploaded)
              client.uploadBookmarkFile(localArchive, "OrganicMaps_Bookmarks_latest.kmz");

            if (uploaded)
            {
              context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
                  .edit()
                  .putLong(KEY_LAST_SYNC_TIMESTAMP, System.currentTimeMillis())
                  .apply();

              if (callback != null)
                callback.onSuccess("Bookmarks backed up successfully to Nextcloud: " + remoteName);
            }
            else if (callback != null)
            {
              callback.onFailure("Failed to upload bookmarks to Nextcloud server.");
            }
          }
          else if (callback != null)
          {
            callback.onFailure("Failed to export bookmarks (Code: " + result.getCode() + ")");
          }
        }
      });

      BookmarkManager.INSTANCE.prepareCategoriesForSharing(catIds, FileType.Kmz);
    });
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    // Default implementation
  }
}
