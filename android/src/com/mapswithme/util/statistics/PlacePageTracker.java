package com.mapswithme.util.statistics;

import android.graphics.Rect;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.taxi.TaxiType;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.UiUtils;

import java.util.List;

public class PlacePageTracker
{
  private static final float VISIBILITY_RATIO_TAXI = 0.6f;
  @NonNull
  private final PlacePageView mPlacePageView;
  @NonNull
  private final View mBottomButtons;
  @NonNull
  private final View mTaxi;
  @Nullable
  private MapObject mMapObject;
  private boolean mTaxiTracked;

  public PlacePageTracker(@NonNull PlacePageView placePageView)
  {
    mPlacePageView = placePageView;
    mBottomButtons = mPlacePageView.findViewById(R.id.pp__buttons);
    mTaxi = mPlacePageView.findViewById(R.id.ll__place_page_taxi);
  }

  public void setMapObject(@Nullable MapObject mapObject)
  {
    mMapObject = mapObject;
  }

  public void onMove()
  {
    trackTaxiVisibility();
  }

  public void onHidden()
  {
    mTaxiTracked = false;
  }

  public void onOpened()
  {
    if (mPlacePageView.getState() == PlacePageView.State.DETAILS)
    {
      Sponsored sponsored = mPlacePageView.getSponsored();
      if (sponsored != null)
        Statistics.INSTANCE.trackSponsoredOpenEvent(sponsored);
    }
  }

  private void trackTaxiVisibility()
  {
    if (!mTaxiTracked && isViewOnScreen(mTaxi, VISIBILITY_RATIO_TAXI) && mMapObject != null)
    {
      List<TaxiType> taxiTypes = mMapObject.getReachableByTaxiTypes();
      if (taxiTypes != null && !taxiTypes.isEmpty())
      {
        String providerName = taxiTypes.get(0).getProviderName();
        Statistics.INSTANCE.trackTaxiEvent(Statistics.EventName.ROUTING_TAXI_REAL_SHOW_IN_PP,
                                           providerName);
        mTaxiTracked = true;
      }
    }
  }

  /**
   *
   * @param visibilityRatio Describes what the portion of view should be visible before
   *                        the view is considered visible on the screen. It can be from 0 to 1.
   */
  private boolean isViewOnScreen(@NonNull View view, float visibilityRatio) {

    if (UiUtils.isInvisible(mPlacePageView))
      return false;

    Rect localRect = new Rect();
    boolean isVisible = view.getGlobalVisibleRect(localRect);
    if (isVisible)
    {
      int visibleHeight = localRect.height() - (localRect.bottom  - mBottomButtons.getTop());
      if ((float)visibleHeight / view.getHeight() >= visibilityRatio)
        return true;
    }
    return false;
  }
}
