package app.organicmaps.widget.placepage;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import androidx.annotation.NonNull;

import androidx.annotation.Nullable;
import app.organicmaps.base.Initializable;
import app.organicmaps.base.Savable;
import app.organicmaps.base.Supportable;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;

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
