package com.mapswithme.maps.background;

import android.app.DownloadManager;
import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;

import com.mapswithme.maps.MwmBroadcastReceiver;
import com.mapswithme.maps.scheduling.JobIdMap;

public class SystemDownloadCompletedReceiver extends MwmBroadcastReceiver
{
  @Override
  public void onReceiveInitialized(@NonNull Context context, @NonNull Intent intent)
  {
    DownloadManager manager = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
    if (manager == null)
      throw new IllegalStateException("Failed to get a download manager");

    intent.setClass(context, SystemDownloadCompletedService.class);
    int jobId = JobIdMap.getId(SystemDownloadCompletedService.class);
    JobIntentService.enqueueWork(context, SystemDownloadCompletedService.class, jobId, intent);
  }
}
