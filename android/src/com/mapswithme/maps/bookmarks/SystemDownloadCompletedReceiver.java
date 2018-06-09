package com.mapswithme.maps.bookmarks;

import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.background.AbstractLogBroadcastReceiver;

public class SystemDownloadCompletedReceiver extends AbstractLogBroadcastReceiver
{
  @NonNull
  @Override
  protected String getAssertAction()
  {
    return DownloadManager.ACTION_DOWNLOAD_COMPLETE;
  }

  @Override
  public void onReceiveInternal(@NonNull Context context, @NonNull Intent intent)
  {
    DownloadManager manager = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
    if (manager == null)
      return;
    intent.setClass(context, SystemDownloadCompletedService.class);
    context.startService(intent);
  }
}
