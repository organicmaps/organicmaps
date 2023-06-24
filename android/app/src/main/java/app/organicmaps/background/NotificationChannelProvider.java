package app.organicmaps.background;

import androidx.annotation.NonNull;

public interface NotificationChannelProvider
{
  @NonNull
  String getDownloadingChannel();

  void setDownloadingChannel();
}
