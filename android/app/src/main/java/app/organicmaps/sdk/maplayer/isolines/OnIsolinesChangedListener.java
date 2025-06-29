package app.organicmaps.sdk.maplayer.isolines;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;

class OnIsolinesChangedListener
{
  @NonNull
  private final Context mContext;
  private IsolinesErrorDialogListener mListener;

  OnIsolinesChangedListener(@NonNull Context app)
  {
    mContext = app;
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void onStateChanged(int type)
  {
    IsolinesState state = IsolinesState.values()[type];
    if (mListener == null)
    {
      state.activate(mContext, null, null);
      return;
    }

    mListener.onStateChanged(state);
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
