package com.mapswithme.maps.content;

import android.app.Application;
import android.content.Context;
import androidx.annotation.NonNull;

public class AbstractContextualListener
{
  @NonNull
  private final Application mApp;

  public AbstractContextualListener(@NonNull Application app)
  {
    mApp = app;
  }

  @NonNull
  public Context getContext()
  {
    return mApp;
  }
}
