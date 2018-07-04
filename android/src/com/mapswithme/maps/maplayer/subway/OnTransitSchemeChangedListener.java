package com.mapswithme.maps.maplayer.subway;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.content.AbstractContextualListener;

public interface OnTransitSchemeChangedListener
{
  @SuppressWarnings("unused")
  @MainThread
  void onTransitStateChanged(int type);

  class Default extends AbstractContextualListener implements OnTransitSchemeChangedListener
  {
    public Default(@NonNull MwmApplication app)
    {
      super(app);
    }

    @Override
    public void onTransitStateChanged(int index)
    {
      MwmApplication app = getApp();
      if (app == null)
        return;

      TransitSchemeState state = TransitSchemeState.makeInstance(index);
      state.onReceived(app);
    }
  }
}
