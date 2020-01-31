package com.mapswithme.maps.maplayer.isolines;

import android.app.Application;

import androidx.annotation.NonNull;
import com.mapswithme.maps.maplayer.subway.OnIsolinesChangedListener;

class OnIsolinesChangedListenerImpl implements OnIsolinesChangedListener
{
  @NonNull
  private final Application mApp;

  OnIsolinesChangedListenerImpl(@NonNull Application app)
  {
    mApp = app;
  }

  @Override
  public void onStateChanged(int type)
  {
    IsolinesState state = IsolinesState.values()[type];
    state.activate(mApp);
  }
}
