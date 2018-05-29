package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class SystemDownloadCompletedReceiver extends BroadcastReceiver
{
  @Override
  public void onReceive(Context context, Intent intent)
  {
    DownloadManager manager = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
    if (manager == null
        || intent == null
        || !DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction()))
    {
      return;
    }

    intent.setClass(context, SystemDownloadCompletedService.class);
    context.startService(intent);
  }
}
