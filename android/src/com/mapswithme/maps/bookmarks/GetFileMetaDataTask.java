package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.Utils;

import java.io.IOException;

class GetFileMetaDataTask extends AsyncTask<Intent, Void, OperationStatus<GetFileMetaDataTask.Result,
    GetFileMetaDataTask.Error>>
{
  @NonNull
  private final DownloadManager mDownloadManager;

  private GetFileMetaDataTask(@NonNull DownloadManager downloadManager)
  {
    mDownloadManager = downloadManager;
  }

  @Override
  protected OperationStatus<Result, Error> doInBackground(Intent... intents)
  {
    Intent intent = intents[0];
    try
    {
      Result result = doInBackgroundInternal(intent);
      return new OperationStatus<>(result, null);
    }
    catch (IOException e)
    {
      return new OperationStatus<>(null, new Error(e));
    }
  }

  @Override
  protected void onPostExecute(OperationStatus<Result, Error> status)
  {
    if (status.isOk())
    {
      Result result = status.getResult();
      BookmarkManager.INSTANCE.importFromCatalog(result.getArchiveId(), result.getFilePath());
    }
  }

  public static GetFileMetaDataTask create(DownloadManager manager)
  {
    return new GetFileMetaDataTask(manager);
  }

  public static class Result
  {
    @Nullable
    private final String mFilePath;
    @Nullable
    private final String mArchiveId;

    public Result(@Nullable String filePath, @Nullable String archiveId)
    {
      mFilePath = filePath;
      mArchiveId = archiveId;
    }

    @Nullable
    public String getFilePath()
    {
      return mFilePath;
    }

    @Nullable
    public String getArchiveId()
    {
      return mArchiveId;
    }
  }

  public static class Error
  {
    @Nullable
    private Throwable mThrowable;

    public Error(@Nullable Throwable throwable)
    {
      mThrowable = throwable;
    }

    @Nullable
    public Throwable getThrowable()
    {
      return mThrowable;
    }
  }

  @NonNull
  private Result doInBackgroundInternal(@NonNull Intent intent) throws IOException
  {
    Cursor cursor = null;
    try
    {
      final long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, 0);
      DownloadManager.Query query = new DownloadManager.Query().setFilterById(id);
      cursor = mDownloadManager.query(query);
      if (cursor.moveToFirst())
      {
        String filePath = getFilePath(cursor);
        String archiveId = getArchiveId(cursor);
        return new Result(filePath, archiveId);
      }
      else
      {
        throw new IOException("Failed to move the cursor at first row");
      }
    }
    finally
    {
      Utils.closeSafely(cursor);
    }
  }

  private String getFilePath(Cursor cursor) throws IOException
  {
    return getColumnValue(cursor, DownloadManager.COLUMN_LOCAL_FILENAME);
  }

  private String getArchiveId(Cursor cursor) throws IOException
  {
    return Uri.parse(getColumnValue(cursor, DownloadManager.COLUMN_URI)).getLastPathSegment();
  }

  @Nullable
  private String getColumnValue(@NonNull Cursor cursor, @NonNull String columnName)
      throws IOException
  {
    int status = cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS));
    if (status == DownloadManager.STATUS_SUCCESSFUL)
    {
      return cursor.getString(cursor.getColumnIndex(columnName));
    }
    else
    {
      throw new IOException(new StringBuilder().append("Failed to download archive, status code = ")
                                               .append(status)
                                               .toString());
    }
  }
}
