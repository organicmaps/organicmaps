package com.mapswithme.maps.background;

import android.app.DownloadManager;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;

import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.downloader.MapDownloadCompletedProcessor;
import com.mapswithme.util.Utils;

public class SystemDownloadCompletedService extends JobIntentService
{
  private interface DownloadProcessor
  {
    boolean process(@NonNull Context context, long id, @NonNull Cursor cursor);
  }

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

    processIntent(manager, intent);
  }

  private void processIntent(@NonNull DownloadManager manager, @NonNull Intent intent)
  {
    Cursor cursor = null;
    try
    {
      final long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, 0);
      DownloadManager.Query query = new DownloadManager.Query().setFilterById(id);
      cursor = manager.query(query);
      if (!cursor.moveToFirst())
        return;

      final DownloadProcessor[] processors = {
          MapDownloadCompletedProcessor::process
      };

      for (DownloadProcessor processor : processors)
      {
        if (processor.process(getApplicationContext(), id, cursor))
          break;
      }
    }
    finally
    {
      Utils.closeSafely(cursor);
    }
  }
}
