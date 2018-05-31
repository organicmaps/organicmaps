package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Error;
import com.mapswithme.maps.bookmarks.data.Result;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;

import java.io.IOException;

public class SystemDownloadCompletedService extends IntentService
{
  public SystemDownloadCompletedService()
  {
    super("GetFileMetaDataService");
  }

  @Override
  protected void onHandleIntent(@Nullable Intent intent)
  {
    DownloadManager manager = (DownloadManager) getSystemService(DOWNLOAD_SERVICE);
    if (manager == null)
      throw new IllegalStateException("Failed to get a download manager");

    final OperationStatus<Result, Error> status = doInBackground(manager, intent);
    UiThread.run(new NotifyCoreTask(getApplicationContext(), status));
  }

  @NonNull
  private OperationStatus<Result, Error> doInBackground(@NonNull DownloadManager manager,
                                                        @Nullable Intent intent)
  {
    if (intent == null)
    {
      NullPointerException npe = new NullPointerException("Intent is null");
      return new OperationStatus<>(null, new Error(npe));
    }
    try
    {
      Result result = doInBackgroundInternal(manager, intent);
      return new OperationStatus<>(result, null);
    }
    catch (IOException e)
    {
      return new OperationStatus<>(null, new Error(e));
    }
  }

  @NonNull
  private static Result doInBackgroundInternal(@NonNull DownloadManager manager,
                                               @NonNull Intent intent) throws IOException
  {
    Cursor cursor = null;
    try
    {
      final long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, 0);
      DownloadManager.Query query = new DownloadManager.Query().setFilterById(id);
      cursor = manager.query(query);
      if (cursor.moveToFirst())
      {
        return new Result(getFilePath(cursor), getArchiveId(cursor));
      }
      throw new IOException("Failed to move the cursor at first row");
    }
    finally
    {
      Utils.closeSafely(cursor);
    }
  }

  @Nullable
  private static String getFilePath(@NonNull Cursor cursor) throws IOException
  {
    return getColumnValue(cursor, DownloadManager.COLUMN_LOCAL_FILENAME);
  }

  @Nullable
  private static String getArchiveId(@NonNull Cursor cursor) throws IOException
  {
    return Uri.parse(getColumnValue(cursor, DownloadManager.COLUMN_URI)).getLastPathSegment();
  }

  @Nullable
  private static String getColumnValue(@NonNull Cursor cursor, @NonNull String columnName)
      throws IOException
  {
    int status = cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS));
    if (status == DownloadManager.STATUS_SUCCESSFUL)
    {
      return cursor.getString(cursor.getColumnIndex(columnName));
    }
    throw new IOException(new StringBuilder().append("Failed to download archive, status code = ")
                                             .append(status)
                                             .toString());
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
      if (mStatus.isOk())
      {
        Result result = mStatus.getResult();
        BookmarkManager.INSTANCE.importFromCatalog(result.getArchiveId(), result.getFilePath());
      }
      else
      {
        Toast.makeText(mAppContext, R.string.download_failed, Toast.LENGTH_SHORT).show();
      }
    }
  }
}
