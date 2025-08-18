package app.organicmaps.sdk.maplayer.isolines;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

class OnIsolinesChangedListener
{
  @Nullable
  private IsolinesErrorDialogListener mListener;

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void onStateChanged(int type)
  {
    if (mListener == null)
      return;
    mListener.onStateChanged(IsolinesState.values()[type]);
  }

  public void attach(@NonNull IsolinesErrorDialogListener listener)
  {
    mListener = listener;
  }

  public void detach()
  {
    mListener = null;
  }
}
