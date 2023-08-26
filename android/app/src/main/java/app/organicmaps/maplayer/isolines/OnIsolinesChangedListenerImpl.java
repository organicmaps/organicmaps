package app.organicmaps.maplayer.isolines;

import android.app.Application;

import androidx.annotation.NonNull;

import app.organicmaps.base.Detachable;

class OnIsolinesChangedListenerImpl implements OnIsolinesChangedListener, Detachable<IsolinesErrorDialogListener>
{
  @NonNull
  private final Application mApp;
  private IsolinesErrorDialogListener mListener;

  OnIsolinesChangedListenerImpl(@NonNull Application app)
  {
    mApp = app;
  }

  @Override
  public void onStateChanged(int type)
  {
    IsolinesState state = IsolinesState.values()[type];
    if (mListener == null)
    {
      state.activate(mApp, null, null);
      return;
    }

    mListener.onStateChanged(state);
  }

  @Override
  public void attach(@NonNull IsolinesErrorDialogListener listener)
  {
    mListener = listener;
  }

  @Override
  public void detach()
  {
    mListener = null;
  }
}
