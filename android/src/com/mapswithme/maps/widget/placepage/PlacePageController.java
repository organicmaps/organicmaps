package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import androidx.annotation.NonNull;

import androidx.annotation.Nullable;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.base.Savable;
import com.mapswithme.maps.base.Supportable;
import com.mapswithme.util.bottomsheet.MenuBottomSheetItem;

import java.util.ArrayList;

public interface PlacePageController extends Initializable<Activity>,
                                             Savable<Bundle>,
                                             Application.ActivityLifecycleCallbacks,
                                             Supportable<PlacePageData>
{
  void openFor(@NonNull PlacePageData data);
  void close(boolean deactivateMapSelection);
  boolean isClosed();
  int getPlacePageWidth();
  @Nullable
  ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems();

  interface SlideListener
  {
    void onPlacePageSlide(int top);
  }
}
