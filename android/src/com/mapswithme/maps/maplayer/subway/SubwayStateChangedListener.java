package com.mapswithme.maps.maplayer.subway;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;

import com.mapswithme.maps.content.AbstractContextualListener;
import com.mapswithme.maps.maplayer.OnTransitSchemeChangedListener;

public class SubwayStateChangedListener extends AbstractContextualListener implements OnTransitSchemeChangedListener
{
  SubwayStateChangedListener(@NonNull Application context)
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
