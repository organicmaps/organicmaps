package com.mapswithme.maps.maplayer.subway;

import android.app.Application;
import android.content.Context;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;

import com.mapswithme.maps.content.AbstractContextualListener;

interface OnTransitSchemeChangedListener
{
  @SuppressWarnings("unused")
  @MainThread
  void onTransitStateChanged(int type);

  class Default extends AbstractContextualListener implements OnTransitSchemeChangedListener
  {
    public Default(@NonNull Application context)
    {
      super(context);
    }

    @Override
    public void onTransitStateChanged(int index)
    {
      Context app = getContext();
      TransitSchemeState state = TransitSchemeState.values()[index];
      state.activate(app);
    }
  }
}
