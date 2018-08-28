package com.mapswithme.maps.downloader;

import android.app.Application;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.Utils;

class ExpandRetryConfirmationListener extends RetryFailedDownloadConfirmationListener
{
  @Nullable
  private final Utils.Proc<Boolean> mDialogClickListener;

  ExpandRetryConfirmationListener(@NonNull Application app,
                                  @Nullable Utils.Proc<Boolean> dialogClickListener)
  {
    super(app);
    mDialogClickListener = dialogClickListener;
  }

  @Override
  public void run()
  {
    super.run();
    if (mDialogClickListener == null)
      return;
    mDialogClickListener.invoke(true);
  }
}
