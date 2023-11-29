package app.organicmaps.maplayer.outdoors;

import android.app.Application;

import androidx.annotation.NonNull;

class OnOutdoorsChangedListener
{
  @NonNull
  private final Application mApp;

  OnOutdoorsChangedListener(@NonNull Application app)
  {
    mApp = app;
  }

}
