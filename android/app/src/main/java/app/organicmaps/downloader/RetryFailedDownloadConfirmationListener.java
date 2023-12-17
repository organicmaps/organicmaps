package app.organicmaps.downloader;

import android.app.Application;

import androidx.annotation.NonNull;

public class RetryFailedDownloadConfirmationListener implements Runnable
{
  @NonNull
  private final Application mApplication;

  RetryFailedDownloadConfirmationListener(@NonNull Application application)
  {
    mApplication = application;
  }

  @Override
  public void run()
  {
    DownloaderNotifier.cancelNotification(mApplication);
  }
}
