package app.organicmaps.downloader;

import android.app.Application;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.util.Utils;

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
