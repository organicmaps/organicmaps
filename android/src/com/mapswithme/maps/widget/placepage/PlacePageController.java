package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import androidx.annotation.NonNull;

import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.base.Savable;
import com.mapswithme.maps.base.Supportable;

public interface PlacePageController extends Initializable<Activity>,
                                             Savable<Bundle>,
                                             Application.ActivityLifecycleCallbacks,
                                             Supportable<PlacePageData>
{
  void openFor(@NonNull PlacePageData data);
  void close(boolean deactivateMapSelection);
  boolean isClosed();
  int getPlacePageWidth();

  interface SlideListener
  {
    void onPlacePageSlide(int top);
  }
}
