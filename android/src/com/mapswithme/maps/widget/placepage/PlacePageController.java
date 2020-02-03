package com.mapswithme.maps.widget.placepage;

import android.app.Application;
import android.os.Bundle;

import androidx.annotation.NonNull;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.base.Savable;

public interface PlacePageController<T> extends Initializable, Savable<Bundle>,
                                             Application.ActivityLifecycleCallbacks
{
  void openFor(@NonNull T object);
  void close();
  boolean isClosed();

  interface SlideListener
  {
    void onPlacePageSlide(int top);
  }
}
