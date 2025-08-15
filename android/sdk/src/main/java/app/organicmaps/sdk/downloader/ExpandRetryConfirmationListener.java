package app.organicmaps.sdk.downloader;

import androidx.annotation.Nullable;
import androidx.core.util.Consumer;

public class ExpandRetryConfirmationListener implements Runnable
{
  @Nullable
  private final Consumer<Boolean> mDialogClickListener;

  public ExpandRetryConfirmationListener(@Nullable Consumer<Boolean> dialogClickListener)
  {
    mDialogClickListener = dialogClickListener;
  }

  @Override
  public void run()
  {
    if (mDialogClickListener == null)
      return;
    mDialogClickListener.accept(true);
  }
}
