package com.mapswithme.maps.maplayer.isolines;

import android.app.Application;

import androidx.annotation.NonNull;
import com.mapswithme.maps.content.AbstractContextualListener;
import com.mapswithme.maps.maplayer.OnTransitSchemeChangedListener;

class IsolinesStateChangedListener extends AbstractContextualListener implements OnTransitSchemeChangedListener
{
  IsolinesStateChangedListener(@NonNull Application app)
  {
    super(app);
  }

  @Override
  public void onTransitStateChanged(int type)
  {
  }
}
