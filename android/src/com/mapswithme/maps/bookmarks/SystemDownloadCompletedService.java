package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.support.annotation.Nullable;

public class SystemDownloadCompletedService extends Service
{
  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    requestArchiveMetadata(intent);
    return START_REDELIVER_INTENT;
  }

  private void requestArchiveMetadata(Intent intent)
  {
    DownloadManager manager = (DownloadManager) getSystemService(DOWNLOAD_SERVICE);
    if (manager != null)
    {
      GetFileMetaDataTask.create(manager).execute(intent);
    }
  }
}
