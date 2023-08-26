package app.organicmaps.maplayer.subway;

import android.app.Application;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;

interface OnTransitSchemeChangedListener
{
  @SuppressWarnings("unused")
  @MainThread
  void onTransitStateChanged(int type);

  class Default implements OnTransitSchemeChangedListener
  {
    @NonNull
    private final Application mContext;

    Default(@NonNull Application context)
    {
      mContext = context;
    }

    @Override
    public void onTransitStateChanged(int index)
    {
      TransitSchemeState state = TransitSchemeState.values()[index];
      state.activate(mContext);
    }
  }
}
