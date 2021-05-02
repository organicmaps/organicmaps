package com.mapswithme.maps.downloader;

import android.app.DownloadManager;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.FileUtils;
import android.text.TextUtils;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MapDownloadCompletedProcessor
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.DOWNLOADER);
  private static final String TAG = MapDownloadCompletedProcessor.class.getSimpleName();

  @WorkerThread
  public static boolean process(@NonNull Context context, long id, @NonNull Cursor cursor)
  {
    try
    {
      String targetUri = cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_URI));
      String targetUriPath = Uri.parse(targetUri).getPath();

      if (TextUtils.isEmpty(targetUriPath) || !MapManager.nativeIsUrlSupported(targetUriPath))
        return false;

      int status = cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS));
      String downloadedFileUri = cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI));

      boolean result = processColumns(context, status, targetUri, downloadedFileUri);
      UiThread.run(new MapDownloadCompletedTask(context, result, id));
      LOGGER.d(TAG, "Processing for map downloading for request id " + id + " is finished.");

      return true;
    }
    catch (Exception e)
    {
      LOGGER.e(TAG, "Failed to process map download for request id " + id + ". Exception ", e);

      return false;
    }
  }

  private static boolean processColumns(@NonNull Context context, int status,
                                        @Nullable String targetUri, @Nullable String localFileUri)
  {
    String targetUriPath = Uri.parse(targetUri).getPath();
    Uri downloadedFileUri = Uri.parse(localFileUri);

    if (status != DownloadManager.STATUS_SUCCESSFUL || TextUtils.isEmpty(targetUriPath)
        || downloadedFileUri == null)
    {
      return false;
    }

    String dstPath = MapManager.nativeGetFilePathByUrl(targetUriPath);
    return copyFile(context, downloadedFileUri, dstPath);
  }

  private static boolean copyFile(@NonNull Context context, @NonNull Uri from, @NonNull String to)
  {
    try (InputStream in = context.getContentResolver().openInputStream(from))
    {
      if (in == null)
        return false;

      try (OutputStream out = new FileOutputStream(to))
      {
        byte[] buf = new byte[4 * 1024];
        int len;
        while ((len = in.read(buf)) > 0)
          out.write(buf, 0, len);

        context.getContentResolver().delete(from, null, null);
        return true;
      }
    }
    catch (IOException e)
    {
      LOGGER.e(TAG, "Failed to copy or delete downloaded map file from " + from.toString() +
                          " to " + to + ". Exception ", e);
      return false;
    }
  }

  private static class MapDownloadCompletedTask implements Runnable
  {
    @NonNull
    private final Context mAppContext;
    private final boolean mStatus;
    private final long mId;

    private MapDownloadCompletedTask(@NonNull Context applicationContext, boolean status, long id)
    {
      mAppContext = applicationContext;
      mStatus = status;
      mId = id;
    }

    @Override
    @MainThread
    public void run()
    {
      MwmApplication application = MwmApplication.from(mAppContext);
      MapDownloadManager manager = MapDownloadManager.from(application);
      manager.onDownloadFinished(mStatus, mId);
    }
  }
}
