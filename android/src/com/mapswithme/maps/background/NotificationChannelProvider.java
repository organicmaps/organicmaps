package com.mapswithme.maps.background;

import android.support.annotation.NonNull;

public interface NotificationChannelProvider
{
  @NonNull
  String getAuthChannel();

  void setAuthChannel();

  @NonNull
  String getDownloadingChannel();

  void setDownloadingChannel();
}
