package com.mapswithme.maps.downloader;

import android.app.DownloadManager;
import android.content.Context;
import android.database.Cursor;

import androidx.annotation.NonNull;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.HashSet;
import java.util.Set;

public class MapDownloadProgressTracker
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.DOWNLOADER);
  private static final String TAG = MapDownloadProgressTracker.class.getSimpleName();

  private static final long PROGRESS_TRACKING_INTERVAL_MILLISECONDS = 1000;

  @NonNull
  private final DownloadManager mDownloadManager;
  @NonNull
  private final Set<Long> mTrackingIds = new HashSet<>();
  private boolean mTrackingEnabled = false;
  private final Runnable mTrackingMethod = this::trackProgress;

  MapDownloadProgressTracker(@NonNull Context context)
  {
    DownloadManager downloadManager =
        (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);

    if (downloadManager == null)
      throw new NullPointerException("Download manager is null, failed to create MapDownloadManager");

    mDownloadManager = downloadManager;
  }

  public void start()
  {
    if (mTrackingEnabled)
      return;

    mTrackingEnabled = true;
    trackProgress();
  }

  public void stop()
  {
    mTrackingEnabled = false;
    UiThread.cancelDelayedTasks(mTrackingMethod);
  }

  public void add(long id)
  {
    mTrackingIds.add(id);
  }

  public void remove(long id)
  {
    mTrackingIds.remove(id);
  }

  private void trackProgress()
  {
    if (!mTrackingEnabled)
      return;

    DownloadManager.Query query = new DownloadManager.Query();
    query.setFilterByStatus(DownloadManager.STATUS_RUNNING);
    Cursor cursor = null;
    try
    {
      cursor = mDownloadManager.query(query);

      cursor.moveToFirst();
      int count = cursor.getCount();
      for (int i = 0; i < count; ++i)
      {
        long id = cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_ID));
        if (!mTrackingIds.contains(id))
          continue;

        long bytesDownloaded = cursor.getLong(cursor.getColumnIndex(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR));
        long bytesTotal = cursor.getLong(cursor.getColumnIndex(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));

        MapManager.nativeOnDownloadProgress(id, bytesDownloaded, bytesTotal);

        cursor.moveToNext();
      }

      UiThread.runLater(mTrackingMethod, PROGRESS_TRACKING_INTERVAL_MILLISECONDS);
    }
    catch (Exception e)
    {
      LOGGER.e(TAG, "Downloading progress tracking failed. Exception: " + e);
      stop();
    }
    finally
    {
      Utils.closeSafely(cursor);
    }
  }
}
