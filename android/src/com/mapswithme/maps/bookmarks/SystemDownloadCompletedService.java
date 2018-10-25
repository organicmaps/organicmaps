package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.JobIntentService;
import android.text.TextUtils;
import android.widget.Toast;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Error;
import com.mapswithme.maps.bookmarks.data.Result;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;

import java.io.IOException;

public class SystemDownloadCompletedService extends JobIntentService
{
  @Override
  public void onCreate()
  {
    super.onCreate();
    MwmApplication app = (MwmApplication) getApplication();
    if (app.arePlatformAndCoreInitialized())
      return;
    app.initCore();
  }

  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    DownloadManager manager = (DownloadManager) getSystemService(DOWNLOAD_SERVICE);
    if (manager == null)
      throw new IllegalStateException("Failed to get a download manager");

    final OperationStatus<Result, Error> status = doInBackground(manager, intent);
    UiThread.run(new NotifyCoreTask(getApplicationContext(), status));
  }

  @NonNull
  private OperationStatus<Result, Error> doInBackground(@NonNull DownloadManager manager,
                                                        @NonNull Intent intent)
  {
    try
    {
      return doInBackgroundInternal(manager, intent);
    }
    catch (Exception e)
    {
      return new OperationStatus<>(null, new Error(e.getMessage()));
    }
  }

  @NonNull
  private static OperationStatus<Result, Error> doInBackgroundInternal(
      @NonNull DownloadManager manager, @NonNull Intent intent) throws IOException
  {
    Cursor cursor = null;
    try
    {
      final long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, 0);
      DownloadManager.Query query = new DownloadManager.Query().setFilterById(id);
      cursor = manager.query(query);
      if (cursor.moveToFirst())
      {

        if (isDownloadFailed(cursor))
        {
          Error error = new Error(getHttpStatus(cursor), getErrorMessage(cursor));
          return new OperationStatus<>(null, error);
        }

        Result result = new Result(getFilePath(cursor), getArchiveId(cursor));
        return new OperationStatus<>(result, null);
      }
      throw new IOException("Failed to move the cursor at first row");
    }
    finally
    {
      Utils.closeSafely(cursor);
    }
  }

  private static boolean isDownloadFailed(@NonNull Cursor cursor)
  {
    int status = cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS));
    return status != DownloadManager.STATUS_SUCCESSFUL;
  }

  @Nullable
  private static String getFilePath(@NonNull Cursor cursor)
  {
    String localUri = getColumnValue(cursor, DownloadManager.COLUMN_LOCAL_URI);
    return localUri == null ? null : Uri.parse(localUri).getPath();
  }

  @Nullable
  private static String getArchiveId(@NonNull Cursor cursor)
  {
    return Uri.parse(getColumnValue(cursor, DownloadManager.COLUMN_URI)).getLastPathSegment();
  }

  @Nullable
  private static String getColumnValue(@NonNull Cursor cursor, @NonNull String columnName)
  {
    return cursor.getString(cursor.getColumnIndex(columnName));
  }

  private static int getHttpStatus(@NonNull Cursor cursor)
  {
    String rawStatus = cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS));
    return Integer.parseInt(rawStatus);
  }

  @Nullable
  private static String getErrorMessage(@NonNull Cursor cursor)
  {
    return cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_REASON));
  }

  private static class NotifyCoreTask implements Runnable
  {
    @NonNull
    private final Context mAppContext;
    @NonNull
    private final OperationStatus<Result, Error> mStatus;

    private NotifyCoreTask(@NonNull Context applicationContext,
                           @NonNull OperationStatus<Result, Error> status)
    {
      mAppContext = applicationContext;
      mStatus = status;
    }

    @Override
    public void run()
    {
      Result result = mStatus.getResult();
      if (mStatus.isOk() && result != null && !TextUtils.isEmpty(result.getArchiveId())
          && !TextUtils.isEmpty(result.getFilePath()))
      {

        BookmarkManager.INSTANCE.importFromCatalog(result.getArchiveId(), result.getFilePath());
        return;
      }

      Error error = mStatus.getError();
      if (error != null)
      {
        if (error.isForbidden())
        {
          Toast.makeText(mAppContext, "Authorization needed. Ui coming soon!",
                         Toast.LENGTH_SHORT).show();
        }
        else
        {
          Toast.makeText(mAppContext, R.string.download_failed, Toast.LENGTH_SHORT).show();
        }
      }
    }
  }
}
