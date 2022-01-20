package com.mapswithme.maps.background;

import androidx.annotation.NonNull;

public interface NotificationChannelProvider
{
  @NonNull
  String getDownloadingChannel();

  void setDownloadingChannel();
}
