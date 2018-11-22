package com.mapswithme.maps.background;

import android.support.annotation.NonNull;

public interface NotificationChannelProvider
{
  @NonNull
  String getUGCChannel();

  void setUGCChannel();

  @NonNull
  String getDownloadingChannel();

  void setDownloadingChannel();
}
