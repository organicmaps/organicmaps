package com.mapswithme.util.statistics;

import android.app.Activity;
import android.graphics.Rect;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.widget.placepage.PlacePageView;

import java.util.List;

import static android.view.View.INVISIBLE;

public class PlacePageTracker
{
  private final int mBottomPadding;
  @NonNull
  private final PlacePageView mPlacePageView;
  @NonNull
  private final View mTaxi;
  @Nullable
  private MapObject mMapObject;

  private boolean mTracked;

  public PlacePageTracker(@NonNull PlacePageView placePageView)
  {
    mPlacePageView = placePageView;
    mTaxi = mPlacePageView.findViewById(R.id.ll__place_page_taxi);
    Activity activity = (Activity) placePageView.getContext();
    mBottomPadding = activity.getResources().getDimensionPixelOffset(R.dimen.place_page_buttons_height);
  }

  public void setMapObject(@Nullable MapObject mapObject)
  {
    mMapObject = mapObject;
  }

  public void onMove()
  {
    trackTaxiVisibility();
  }

  public void onHide()
  {
    mTracked = false;
  }

  private void trackTaxiVisibility()
  {
    if (!mTracked && isViewOnScreen(mTaxi) && mMapObject != null)
    {
      List<Integer> taxiTypes = mMapObject.getReachableByTaxiTypes();
      if (taxiTypes != null && !taxiTypes.isEmpty())
      {
        @TaxiManager.TaxiType
        int type = taxiTypes.get(0);
        Statistics.INSTANCE.trackTaxiEvent(Statistics.EventName.ROUTING_TAXI_REAL_SHOW_IN_PP, type);
        mTracked = true;
      }
    }
  }

  private boolean isViewOnScreen(@NonNull View view) {

    if (mPlacePageView.getVisibility() == INVISIBLE)
      return false;

    Rect localRect = new Rect();
    Rect globalRect = new Rect();
    view.getLocalVisibleRect(localRect);
    view.getGlobalVisibleRect(globalRect);
    return localRect.bottom >= view.getHeight() && localRect.top == 0
           && globalRect.bottom <= mPlacePageView.getBottom() - mBottomPadding;
  }
}
