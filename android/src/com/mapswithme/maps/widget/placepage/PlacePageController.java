package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import androidx.annotation.NonNull;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.base.Savable;
import com.mapswithme.maps.base.Supportable;

public interface PlacePageController<T> extends Initializable<Activity>, Savable<Bundle>,
                                                Application.ActivityLifecycleCallbacks,
                                                Supportable<T>
{
  void openFor(@NonNull T object);
  void close(boolean deactivateMapSelection);
  boolean isClosed();

  interface SlideListener
  {
    void onPlacePageSlide(int top);
  }
}
